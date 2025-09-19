#pragma once

// ===================== Networking / OTA =====================
#define WIFI_SSID "ZetaReticula"
#define WIFI_PASSWORD "T#2gYwMuuA"

#define STATIC_IP      IPAddress(172, 16, 2, 231)
#define STATIC_GATEWAY IPAddress(172, 16, 2, 1)
#define STATIC_SUBNET  IPAddress(255, 255, 255, 0)
#define STATIC_DNS1    IPAddress(172, 16, 4, 7)
#define STATIC_DNS2    IPAddress(1, 1, 1, 1)

#define OTA_HOSTNAME "Marvin-4-ESP32S3"
#define OTA_PASSWORD "marvinOTA2025"
#define OTA_PORT 3232

// ===================== Debug / App =====================
#define DEBUG_LEVEL 2
#define ENABLE_SERIAL_PLOT 0

// Detection behavior
#define DETECTION_COOLDOWN_MS 1000
#define COMMAND_LISTEN_DURATION_SEC 5
#define COMMAND_CONFIDENCE_THRESHOLD 0.30f

// ===================== Hardware Pins (ESP32-S3 DevKitC-1) =====================
// I2S Microphone (INMP441)
#define I2S_BCLK_PIN 18
#define I2S_LRCL_PIN 10
#define I2S_DOUT_PIN 21

// I2C (AHT10)
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Outputs
#define LED_PIN 2
#define BUZZER_PIN 41

// ===================== Wake Word Model =====================
#define WAKE_CLASS_INDEX   0
#define WAKE_PROB_THRESH   0.30f

// Buzzer (LEDC)
#define BUZZER_CHANNEL 0

// smoke test mode
#define MIC_SMOKE_TEST 0  // Set to 0 to disable, 1 to enable

// Notes: AP_FRAME_SAMPLES / AP_HOP_SAMPLES come from frontend_params.h only.
