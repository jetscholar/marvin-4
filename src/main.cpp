#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "esp_heap_caps.h"

#include "env.h"					// WIFI_SSID / WIFI_PASS
#include "frontend_params.h"		// KWS_FRAMES, KWS_NUM_MFCC, KWS_NUM_CLASSES
#include "AudioProcessor.h"
#include "WakeWordDetector.h"

// ---------- Mem logging ----------
static void logMem() {
	size_t psram_free  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
	size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
	size_t dram_free   = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
	Serial.printf("PSRAM total: %u, free: %u | DRAM free: %u\n",
	              (unsigned)psram_total, (unsigned)psram_free, (unsigned)dram_free);
}

// ---------- Globals ----------
AudioProcessor processor;
WakeWordDetector detector(processor);	// pass by reference (NOT pointer)
Adafruit_AHTX0 aht;

static uint32_t lastSensorMs = 0;

// ---------- WiFi / OTA ----------
static void connectWiFi() {
	WiFi.mode(WIFI_STA);
	WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS1, STATIC_DNS2);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	Serial.print("Connecting to WiFi");
	unsigned long t0 = millis();
	while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
		delay(300);
		Serial.print(".");
	}
	Serial.println();
	if (WiFi.status() == WL_CONNECTED) {
		Serial.printf("✅ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
	} else {
		Serial.println("❌ WiFi connect timeout");
	}
}


static void setupOTA() {
	ArduinoOTA.setHostname("Marvin-4-ESP32S3");
	ArduinoOTA.onStart([](){ Serial.println("OTA Start"); });
	ArduinoOTA.onEnd([](){ Serial.println("\nOTA End"); });
	ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
		Serial.printf("OTA Progress: %u%%\r", (p * 100) / t);
	});
	ArduinoOTA.onError([](ota_error_t err){
		Serial.printf("OTA Error[%u]\n", (unsigned)err);
	});
	ArduinoOTA.begin();
	Serial.printf("🔧 OTA ready on %s.local:3232\n", "Marvin-4-ESP32S3");
}

// ---------- Setup ----------
void setup() {
	Serial.begin(115200);
	delay(200);
	logMem();

	connectWiFi();
	setupOTA();

	// I2C (AHT10)
	Wire.begin(8, 9, 100000);
	if (!aht.begin(&Wire)) {
		Serial.println("⚠️ AHT10 not found");
	} else {
		Serial.println("✅ AHT10 detected");
	}

	// Audio / I2S
	if (!processor.begin()) {
		Serial.println("❌ AudioProcessor begin() failed");
	} else {
		Serial.println("✅ INMP441 I2S initialized");
	}

	// KWS / Detector
	if (!detector.init()) {
		Serial.println("❌ WakeWordDetector init failed");
	} else {
		Serial.printf("KWS fs=%dHz frames=%d mfcc=%d mel=%d classes=%d idx=%d thr=%.3f\n",
			16000, (int)KWS_FRAMES, (int)KWS_NUM_MFCC, 40, (int)KWS_NUM_CLASSES, 0, detector.getThreshold());
		Serial.println("✅ WakeWordDetector ready");
	}
}

// ---------- Loop ----------
void loop() {
	ArduinoOTA.handle();

	// Run one inference step (fills prob and smoothed avg)
	float p = 0.f, avg = 0.f;
	const bool wake = detector.detect_once(p, avg);
	if (wake) {
		Serial.println("🎉 Wake word detected!");
		// TODO: trigger your action here
	}

	// Periodic sensor read (every ~2s)
	const uint32_t now = millis();
	if (now - lastSensorMs > 2000) {
		lastSensorMs = now;
		sensors_event_t hum, temp;
		if (aht.getEvent(&hum, &temp)) {
			Serial.printf("🌡️ Temp: %.2f°C  💧 Humidity: %.2f%%\n", temp.temperature, hum.relative_humidity);
		}
	}

	// Small delay to avoid pegging the CPU; detector itself logs at ~1Hz internally
	delay(10);
}






