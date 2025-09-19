#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <AHT10.h>
#include <ArduinoOTA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "env.h"
#include "frontend_params.h"

#include "AudioCapture.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"
#include "WakeWordDetector.h"

// ====== Globals ======
static AudioCapture		g_cap;
static AudioProcessor	g_proc;
static ManualDSCNN		g_net;
static WakeWordDetector g_det(g_cap, g_proc, g_net);

static AHT10 g_aht10(AHT10_ADDRESS_0X38);

static TaskHandle_t task_loop = nullptr;
static volatile bool ota_active = false;

// ====== WiFi & OTA ======
static void connectWiFi() {
	Serial.println("Connecting to WiFi.");
	WiFi.mode(WIFI_STA);
	WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS1, STATIC_DNS2);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) delay(250);
	Serial.printf("✅ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
	Serial.flush();
}

static void setupOTA() {
	ArduinoOTA.setPort(OTA_PORT);
	ArduinoOTA.setHostname(OTA_HOSTNAME);
	ArduinoOTA.setPassword(OTA_PASSWORD);
	ArduinoOTA.onStart([]() {
		ota_active = true;
		if (task_loop) vTaskDelete(task_loop);
		Serial.println("OTA Start");
		Serial.flush();
	});
	ArduinoOTA.onEnd([](){ Serial.println("\nOTA End"); Serial.flush(); });
	ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
		Serial.printf("OTA Progress: %u%%\r", (p / (t / 100)));
		Serial.flush();
	});
	ArduinoOTA.onError([](ota_error_t e){
		Serial.printf("OTA Error[%u]\n", e);
		Serial.flush();
	});
	ArduinoOTA.begin();
	Serial.printf("🔧 OTA ready on %s:%d\n", OTA_HOSTNAME, OTA_PORT);
	Serial.flush();
}

// ====== Worker Task ======
static void kwsTask(void* param) {
	float p_conf, p_avg;
	bool fired = false;
	while (1) {
		unsigned long start = millis();
		Serial.println("DEBUG: kwsTask loop");
		UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(nullptr);
		Serial.printf("DEBUG: kwsTask stack high water mark: %u bytes\n", stackHighWater * sizeof(StackType_t));
		Serial.printf("DEBUG: Free heap: %u bytes\n", ESP.getFreeHeap());
		if (g_det.detect_once(p_conf, p_avg)) {
			fired = true;
		}
		Serial.printf("DEBUG: kwsTask p_conf=%.4f p_avg=%.4f fired=%d, duration=%lu ms\n", p_conf, p_avg, fired, millis() - start);
		Serial.flush();
		if (fired) {
			digitalWrite(LED_PIN, HIGH);
			ledcWriteTone(BUZZER_CHANNEL, 1000);
			delay(200);
			digitalWrite(LED_PIN, LOW);
			ledcWriteTone(BUZZER_CHANNEL, 0);
			delay(DETECTION_COOLDOWN_MS);
			fired = false;
		}
		vTaskDelay(pdMS_TO_TICKS(100));  // Increased to 100ms
	}
}

// ====== Arduino ======
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    ledcSetup(BUZZER_CHANNEL, 2000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

    connectWiFi();
    setupOTA();

    // I2C (AHT10)
    Wire.end();  // Reset I2C bus
    delay(500);  // Longer delay for bus reset
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);
    int retries = 3;
    while (retries-- > 0) {
        if (g_aht10.begin()) {
            Serial.println("✅ AHT10 detected at 0x38");
            break;
        }
        Serial.println("❌ AHT10 not detected at 0x38");
        g_aht10 = AHT10(AHT10_ADDRESS_0X39);
        if (g_aht10.begin()) {
            Serial.println("✅ AHT10 detected at 0x39");
            break;
        }
        Serial.println("❌ AHT10 not detected at 0x39");
        delay(500);
    }
    if (retries <= 0) {
        Serial.println("DEBUG: Scanning I2C bus...");
        for (uint8_t addr = 1; addr <= 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("DEBUG: I2C device found at 0x%02X\n", addr);
            }
        }
    }
    Serial.flush();

    if (g_aht10.begin()) {
        Wire.beginTransmission(0x38);
        Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
        if (Wire.endTransmission() == 0) {
            delay(80);
            Wire.requestFrom(0x38, 6);
            if (Wire.available() >= 6) {
                uint8_t buf[6];
                for (int i = 0; i < 6; ++i) buf[i] = Wire.read();
                uint32_t raw_h = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
                uint32_t raw_t = ((uint32_t)buf[3] & 0x0F) << 16 | ((uint32_t)buf[4] << 8) | buf[5];
                float h = 100.0f * raw_h / 1048576.0f;
                float t = 200.0f * raw_t / 1048576.0f - 50.0f;
                Serial.printf("MANUAL: Raw T=%.2f°C H=%.2f%%\n", t, h);
            } else {
                Serial.println("MANUAL: Short read");
            }
        } else {
            Serial.println("MANUAL: Trigger failed");
        }
        Serial.flush();
    }

    #if MIC_SMOKE_TEST
    Serial.println("=== MIC SMOKE TEST START ===");
    if (!g_cap.begin()) {
        Serial.println("❌ I2S init failed for smoke test");
        while (true) delay(1000);
    }
    Serial.println("✅ I2S initialized for smoke test");
    int16_t pcm[AP_FRAME_SAMPLES];
    unsigned long start_time = millis();
    while (millis() - start_time < 5000) {  // 5 seconds
        if (g_cap.readFrame(pcm)) {
            int32_t sum_abs = 0;
            for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
                sum_abs += abs(pcm[i]);
            }
            Serial.printf("SMOKE: PCM sum_abs=%d, sample[0]=%d\n", sum_abs, pcm[0]);
            digitalWrite(LED_PIN, HIGH); delay(10); digitalWrite(LED_PIN, LOW);  // Blink LED
        } else {
            Serial.println("SMOKE: readFrame failed");
        }
        delay(100);  // 10 Hz sampling
    }
    Serial.println("=== MIC SMOKE TEST END ===");
    #endif

    // Audio path
    if (!g_cap.begin()) {
        Serial.println("❌ I2S init failed"); while (true) delay(1000);
    }
    Serial.println("✅ INMP441 I2S initialized");
    if (!g_proc.begin()) {
        Serial.println("❌ AudioProcessor init failed"); while (true) delay(1000);
    }
    if (!g_net.begin()) {
        Serial.println("❌ ManualDSCNN init failed"); while (true) delay(1000);
    }
    g_det.begin();

    Serial.printf("KWS fs=%dHz frames=%d mfcc=%d mel=%d classes=%d idx=%d thr=%.3f\n",
                  KWS_SAMPLE_RATE_HZ, KWS_FRAMES, KWS_NUM_MFCC, KWS_NUM_MEL, KWS_NUM_CLASSES,
                  WAKE_CLASS_INDEX, WAKE_PROB_THRESH);
    Serial.flush();

    // Start detection task with increased stack
    xTaskCreatePinnedToCore(kwsTask, "kwsTask", 16384, nullptr, 1, &task_loop, 1);
}

unsigned long last_env = 0;

void loop() {
	ArduinoOTA.handle();
	if (ota_active) { delay(1000); return; }

	const unsigned long now = millis();
	if (now - last_env >= 2000) {
		last_env = now;
		const float t = g_aht10.readTemperature();
		const float h = g_aht10.readHumidity();
		if (t != AHT10_ERROR && h != AHT10_ERROR) {
			Serial.printf("🌡️ Temp: %.2f°C  💧 Humidity: %.2f%%\n", t, h);
		} else {
			Serial.println("DEBUG: AHT10 read failed");
		}
		Serial.flush();
	}
	delay(10);
}



