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
#define DEBUG_LEVEL 3
#define ENABLE_SERIAL_PLOT 0

// Detection behavior (cooldown etc). Threshold comes from frontend_params.h
#define DETECTION_COOLDOWN_MS 2000
#define COMMAND_LISTEN_DURATION_SEC 5
#define COMMAND_CONFIDENCE_THRESHOLD 0.30f

// ===================== Hardware Pins (ESP32-S3 DevKitC-1) =====================
// I2S Microphone (INMP441)
#define I2S_BCLK_PIN 4
#define I2S_LRCL_PIN 5
#define I2S_DOUT_PIN 6

// I2C (AHT10)
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Outputs
#define LED_PIN 2
#define BUZZER_PIN 41

// ===================== Notes =====================
// Do NOT define MFCC/MEL/FFT sizes here.
// The model’s exported frontend_params.h provides:
//   KWS_SAMPLE_RATE_HZ, KWS_FRAME_MS, KWS_STRIDE_MS
//   KWS_NUM_MEL, KWS_NUM_MFCC, KWS_FRAMES
//   KWS_NUM_CLASSES, KWS_LABEL_MARVIN_IDX, KWS_TRIGGER_THRESHOLD
