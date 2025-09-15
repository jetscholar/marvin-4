#pragma once

// WiFi Configuration (for OTA and future networking)
#define WIFI_SSID "ZetaReticula"
#define WIFI_PASSWORD "T#2gYwMuuA"

#define STATIC_IP      IPAddress(172, 16, 2, 231)   // Static IP address
#define STATIC_GATEWAY IPAddress(172, 16, 2, 1)     // Gateway
#define STATIC_SUBNET  IPAddress(255, 255, 255, 0)  // Subnet mask
#define STATIC_DNS1    IPAddress(172, 16, 4, 7)     // Primary DNS (Pi-hole)
#define STATIC_DNS2    IPAddress(1, 1, 1, 1)        // Secondary DNS (Cloudflare)

#define OTA_HOSTNAME "Marvin-4-ESP32S3"
#define OTA_PASSWORD "marvinOTA2025"
#define OTA_PORT 3232

// Debug Settings
#define DEBUG_LEVEL 3
#define ENABLE_SERIAL_PLOT 0

// Wake Word Settings
#define WAKE_WORD_THRESHOLD 0.340f
#define SAMPLE_RATE 16000
#define FRAME_DURATION_MS 30
#define STRIDE_MS 15

// MFCC Configuration (Enhanced for ESP32-S3 with PSRAM)
#define MFCC_NUM_COEFFS 10
#define MFCC_NUM_FRAMES 65    // Matches notebook's 65 frames
#define WINDOW_SIZE 512
#define HOP_LENGTH 240        // Adjusted for 65 frames

// Detection Settings
#define DETECTION_COOLDOWN_MS 2000
#define COMMAND_LISTEN_DURATION_SEC 5
#define COMMAND_CONFIDENCE_THRESHOLD 0.3f

// ===== CORRECTED GPIO PINS FOR ESP32-S3-DevKitC-1 =====

// I2S Microphone (INMP441) - Safe I2S-capable pins
#define I2S_BCLK_PIN 4        // Bit clock
#define I2S_LRCL_PIN 5        // Word select
#define I2S_DOUT_PIN 6        // Serial data

// I2C Bus (AHT10) - ESP32-S3 defaults
#define I2C_SDA_PIN 8         // Data line
#define I2C_SCL_PIN 9         // Clock line

// Output Devices
#define LED_PIN 2             // LED (PWM-capable)
#define BUZZER_PIN 41          // Buzzer (PWM-capable)

// ===== ESP32-S3 SPECIFIC OPTIMIZATIONS =====

// PSRAM Configuration
#define ENABLE_PSRAM 1
#define AUDIO_BUFFER_SIZE 4096    // 4KB buffer, leverages PSRAM
#define MODEL_ARENA_SIZE 32768    // 32KB arena (sufficient for TFLM)

// Dual-core optimization
#define AUDIO_CORE 0              // Core 0 for audio processing
#define INFERENCE_CORE 1          // Core 1 for ML inference

// Enhanced audio processing with more memory
#define FFT_SIZE 1024             // Matches arduinoFFT capability
#define MEL_FILTER_BANKS 26       // Standard mel-scale banks
#define PRE_EMPHASIS 0.97f        // Audio pre-processing

// Command recognition settings
#define MAX_COMMANDS 8            // Support for multiple commands
#define COMMAND_BUFFER_MS 3000    // 3-second command buffer