#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <AHT10.h>
#include <ArduinoOTA.h>

#include "env.h"
#include "frontend_params.h"

#include "AudioCapture.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"
#include "WakeWordDetector.h"

// ====== Globals ======
static AudioCapture   g_cap;
static AudioProcessor g_proc;
static ManualDSCNN    g_net;
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
}

static void setupOTA() {
	ArduinoOTA.setPort(OTA_PORT);
	ArduinoOTA.setHostname(OTA_HOSTNAME);
	ArduinoOTA.setPassword(OTA_PASSWORD);
	ArduinoOTA.onStart([]() {
		ota_active = true;
		if (task_loop) vTaskDelete(task_loop);
		Serial.println("OTA Start");
	});
	ArduinoOTA.onEnd([](){ Serial.println("\nOTA End"); });
	ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
		Serial.printf("OTA Progress: %u%%\r", (p / (t / 100)));
	});
	ArduinoOTA.onError([](ota_error_t e){
		Serial.printf("OTA Error[%u]\n", e);
	});
	ArduinoOTA.begin();
	Serial.printf("🔧 OTA ready on %s:%d\n", OTA_HOSTNAME, OTA_PORT);
}

// ====== Worker Task ======
static void kwsTask(void*) {
	for (;;) {
		float p, avg;
		const bool fired = g_det.detect_once(p, avg);
		Serial.printf("p=%.3f avg=%.3f\n", p, avg);
		if (fired) {
			digitalWrite(LED_PIN, HIGH);
			delay(200);
			digitalWrite(LED_PIN, LOW);
			delay(DETECTION_COOLDOWN_MS);
		}
		delay(5);
	}
}

// ====== Arduino ======
void setup() {
	Serial.begin(115200);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	connectWiFi();
	setupOTA();

	// I2C (AHT10)
	Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
	if (g_aht10.begin() == true) {
		Serial.println("✅ AHT10 detected");
	} else {
		Serial.println("❌ AHT10 not detected");
	}

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

	// Start detection task
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
		}
	}
	delay(10);
}





