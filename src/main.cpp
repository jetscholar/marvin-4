#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "driver/i2s.h"

#include "env.h"
#include "AudioProcessor.h"
#include "WakeWordDetector.h"

// ========== Globals ==========
AudioProcessor processor;
WakeWordDetector detector(&processor);
Adafruit_AHTX0 aht;

// Audio frame buffer
static int16_t audio_frame[WINDOW_SIZE];

// ========== I2S setup ==========
void setupI2S() {
	const i2s_config_t i2s_config = {
		.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
		.sample_rate = SAMPLE_RATE,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
		.communication_format = I2S_COMM_FORMAT_STAND_I2S,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
		.dma_buf_count = 4,
		.dma_buf_len = 256,
		.use_apll = false,
		.tx_desc_auto_clear = false,
		.fixed_mclk = 0
	};

	const i2s_pin_config_t pin_config = {
		.bck_io_num = I2S_BCLK_PIN,
		.ws_io_num = I2S_LRCL_PIN,
		.data_out_num = I2S_PIN_NO_CHANGE,
		.data_in_num = I2S_DOUT_PIN
	};

	i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
	i2s_set_pin(I2S_NUM_0, &pin_config);
	i2s_zero_dma_buffer(I2S_NUM_0);
}

// ========== Helpers ==========
void beepAndFlash() {
	digitalWrite(LED_PIN, HIGH);
	digitalWrite(BUZZER_PIN, HIGH);
	delay(100);
	digitalWrite(LED_PIN, LOW);
	digitalWrite(BUZZER_PIN, LOW);
}

// ========== Setup ==========
void setup() {
	Serial.begin(115200);
	delay(200);

	pinMode(LED_PIN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	// WiFi
	WiFi.mode(WIFI_STA);
	WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS1, STATIC_DNS2);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.print("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.printf("\n✅ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

	// OTA
	ArduinoOTA.setHostname(OTA_HOSTNAME);
	ArduinoOTA.setPassword(OTA_PASSWORD);
	ArduinoOTA.begin();
	Serial.printf("🔧 OTA ready on %s.local:%d\n", OTA_HOSTNAME, OTA_PORT);

	// I2C (AHT10)
	Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
	if (!aht.begin()) {
		Serial.println("⚠️ AHT10 not found");
	} else {
		Serial.println("✅ AHT10 detected");
	}

	// I2S mic
	setupI2S();
	Serial.println("✅ INMP441 I2S initialized");

	// Detector
	if (!detector.init()) {
		Serial.println("❌ WakeWordDetector init failed");
	} else {
		Serial.println("✅ WakeWordDetector ready");
	}
}

// ========== Loop ==========
void loop() {
	ArduinoOTA.handle();

	// 1) Read audio samples into frame
	size_t bytes_read = 0;
	i2s_read(I2S_NUM_0, audio_frame, sizeof(audio_frame), &bytes_read, portMAX_DELAY);
	if (bytes_read < sizeof(audio_frame)) {
		Serial.println("⚠️ Short I2S read");
		return;
	}

	// 2) Feed to processor
	processor.processFrame(audio_frame);

	// 3) Run detection
	float conf = detector.detect();
	float avg  = detector.getAverageConfidence();
	Serial.printf("Frame=%.3f Avg=%.3f\n", conf, avg);

	// 4) Feedback if detected
	if (conf > detector.getThreshold()) {
		beepAndFlash();
	}

	// 5) Sensor telemetry (every second)
	static unsigned long last_sensor = 0;
	if (millis() - last_sensor > 1000) {
		sensors_event_t hum, temp;
		aht.getEvent(&hum, &temp);
		Serial.printf("🌡️ Temp: %.2f°C  💧 Humidity: %.2f%%\n",
			temp.temperature, hum.relative_humidity);
		last_sensor = millis();
	}
}




