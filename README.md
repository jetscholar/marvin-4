# Marvin-4: ESP32-S3 Wake Word + Voice Command System

**Marvin-4** is the next iteration of the Marvin project, extended from [Marvin-3](https://github.com/jetscholar/marvin-3).  
It runs on the **ESP32-S3-DevKitC-1 (N16R8)** and combines wake word detection, voice command recognition, and environmental sensing with buzzer/LED feedback.

---

## ✨ Features

- Wake word detection **("marvin")** with 99.69% validation accuracy  
- **<40ms inference latency** on ESP32-S3  
- **AHT10** temp/humidity sensor integration  
- **INMP441** I²S microphone capture  
- **Passive buzzer + LED** feedback  
- Voice command support via **TensorFlow Lite Micro**  
- Optimized for **512KB SRAM + PSRAM**

---

## 🛠️ Hardware

- **Board:** ESP32-S3-DevKitC-1 (N16R8, 16MB flash)  
- **Mic:** INMP441 (I²S)  
  - BCLK = GPIO4  
  - WS   = GPIO5  
  - DOUT = GPIO6  
- **Sensor:** AHT10 (I²C, SDA=GPIO8, SCL=GPIO9)  
- **Buzzer:** TZT TDK PS1240 (PWM, GPIO38)  
- **LED:** GPIO2 (via 220 Ω resistor)

---

## ⚙️ Setup

1. Clone repository and open as PlatformIO project.  
2. Copy model artifacts (`model_int8.cc`, `frontend_params.h`) from Marvin-3 into `/lib/`.  
3. Install dependencies:

```sh
   pio lib install
