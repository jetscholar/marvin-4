#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include "env.h"
#include "AudioCapture.h"
#include <Adafruit_AHTX0.h>
#include "WakeWordDetector.h"

// ===== Objects =====
AudioCapture audio;
Adafruit_AHTX0 aht;
WakeWordDetector detector(audio);

const int ledChannel = 0;
const int buzzerChannel = 1;

void setup() {
    Serial.begin(115200);
    Serial.println("\n🤖 Marvin-4 Wake Word Test - Starting");

    // --- WiFi setup ---
    if (!WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS1, STATIC_DNS2)) {
        Serial.println("⚠️ Failed to configure static IP!");
    }
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi connected! IP: " + WiFi.localIP().toString());

    // --- OTA setup ---
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.begin();
    Serial.printf("🔧 OTA ready on port %d\n", OTA_PORT);

    // --- I2C + AHT10 setup ---
    if (!aht.begin(&Wire)) {
        Serial.println("❌ AHT10 not detected!");
    } else {
        Serial.println("✅ AHT10 detected");
    }

    // --- I2S mic setup ---
    if (!audio.begin()) {
        Serial.println("❌ INMP441 I2S setup failed!");
    } else {
        Serial.println("✅ INMP441 I2S initialized");
    }

    // --- LED + Buzzer setup ---
    ledcSetup(ledChannel, 1000, 8);    // LED: 1 kHz, 8-bit
    ledcAttachPin(LED_PIN, ledChannel);

    ledcSetup(buzzerChannel, 4000, 8); // Buzzer: base 4 kHz, 8-bit
    ledcAttachPin(BUZZER_PIN, buzzerChannel);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.println("✅ LED and Buzzer pins configured");

    // --- Start Wake Word Detection ---
    detector.start();
}

void loop() {
    ArduinoOTA.handle(); // Handle OTA requests

    // --- Test Wake Word ---
    if (detector.detect()) {
        Serial.println("🎉 Wake word action triggered!");
        digitalWrite(LED_PIN, HIGH);
        ledcWriteTone(buzzerChannel, 2000); // Alert tone
        delay(500);
        ledcWrite(buzzerChannel, 0);
        digitalWrite(LED_PIN, LOW);
        delay(DETECTION_COOLDOWN_MS); // Cooldown
    }

    // --- Test AHT10 ---
    sensors_event_t humidity, temp;
    if (aht.getEvent(&humidity, &temp)) {
        Serial.printf("🌡️ Temp: %.2f°C  💧 Humidity: %.2f%%\n",
                        temp.temperature, humidity.relative_humidity);
    } else {
        Serial.println("❌ Failed to read AHT10");
    }

    // --- Test INMP441 ---
    Serial.println("🎙️ Testing INMP441...");
    int16_t sample = audio.readSample();
    if (sample != 0) {
        Serial.printf("✅ INMP441 Sample: %d\n", sample);
    } else {
        Serial.println("❌ INMP441 no sample detected!");
    }

    delay(1000); // Reduced delay for real-time detection
}

