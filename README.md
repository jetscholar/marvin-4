# Marvin-4 — ESP32-S3 Wake-Word Sensor (OTA)

An always-on, OTA-updatable wake-word device built on **ESP32-S3 DevKitC-1**.
It listens for your custom wake word (“**marvin**” by default), exposes system telemetry, and beeps an external passive buzzer (via NPN driver) on detection.

* ✅ **On-device KWS** (MFCC frontend + compact DSCNN inference)
* ✅ **INMP441 I2S microphone** (mono: L/R tied to GND → left channel)
* ✅ **AHT10 temperature / humidity**
* ✅ **Wi-Fi (static IP optional) + Arduino OTA**
* ✅ **PSRAM enabled builds**
* ✅ **Deterministic model export** (`models/model_weights_float.h`)
* ✅ **Label mapping in one place** (`include/labels.h`)
* ✅ **Buzzer driver on GPIO 41 via 2N/PN2222/SS8050 NPN**

---

## 🔧 Hardware

* **Board:** ESP32-S3 DevKitC-1 (N16R8 recommended: 16MB Flash + 8MB PSRAM)
* **Mic:** INMP441 (PDM/I2S)

  * **BCLK:** GPIO 4
  * **LRCL:** GPIO 5
  * **DOUT:** GPIO 6
  * **L/R:** Tie to **GND** (force left channel)
* **I²C (AHT10):** SDA=GPIO 8, SCL=GPIO 9
* **Outputs:** LED=GPIO 2, **Buzzer=GPIO 41** → NPN (e.g., 2N3904/SS8050) → passive buzzer

> Buzzer is **active-HIGH** at MCU pin; drive the transistor base via series resistor (e.g., 1–4.7 kΩ), with buzzer to +3V3 and transistor sinking to GND. Add a small diode if you swap to an inductive transducer later.

---

## 🌲 Project Structure

```
marvin-4/
├─ platformio.ini
├─ src/
│  └─ main.cpp
├─ include/
│  ├─ env.h                  # Wi-Fi/OTA pins & app constants (tabs indentation)
│  ├─ frontend_params.h      # Exported KWS frontend config (rate, frames, mfcc…)
│  └─ labels.h               # Label order (index 0 must match wake class)
├─ lib/
│  ├─ AudioCapture/
│  │  ├─ AudioCapture.h
│  │  └─ AudioCapture.cpp    # I2S init + 16-kHz mono read → int16_t frames
│  ├─ AudioProcessor/
│  │  ├─ AudioProcessor.h
│  │  ├─ AudioProcessor.cpp  # Window/Mel/DCT, MFCC, normalization, ring buffer
│  │  └─ mfcc_norm.h         # MFCC_NORM_SCALE / MFCC_NORM_OFFSET
│  ├─ ManualDSCNN/
│  │  ├─ ManualDSCNN.h
│  │  └─ ManualDSCNN.cpp     # Conv/BN/Pool/1×1/GAP/Dense/Softmax primitives
│  └─ WakeWordDetector/
│     ├─ WakeWordDetector.h
│     └─ WakeWordDetector.cpp # Glue: capture → MFCC stack → model → decision
└─ models/
   └─ model_weights_float.h   # Exported weights/BN params from training
```

---

## ⚙️ Build & Flash (PlatformIO)

`platformio.ini` (key settings already present):

* `board = esp32-s3-devkitc-1`
* `framework = arduino`
* PSRAM + **qio\_opi** memory config
* `build_flags` include: `-Iinclude`, `-Imodels`, `-O2`, `-ffast-math`, `-DBOARD_HAS_PSRAM`

**Lib deps**

```ini
lib_deps =
    kosme/arduinoFFT@^2.0.4
    enjoyneering/AHT10@^1.1.0
```

**First flash via USB**

```bash
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor
```

---

## 🚀 Quick Start

1. **Wire hardware** exactly as listed above (confirm mic L/R → GND).
2. **Place model export** at `models/model_weights_float.h`.
3. **Set app config** in `include/env.h`:

   * Wi-Fi SSID/PASS (or leave as your current values)
   * Static IP (optional)
   * `WAKE_CLASS_INDEX` (usually `0`) and `WAKE_PROB_THRESH` (start low \~0.30)
4. **Labels:** Verify `include/labels.h` first element matches your wake word:

   ```cpp
   // labels.h
   #pragma once
   static const char* KWS_LABELS[] = { "marvin", "_unknown_", "_silence_" };
   ```
5. **Build/flash**; open serial monitor at **115200**.

On boot, you should see:

```
✅ WiFi connected! IP: ...
🔧 OTA ready on Marvin-4-ESP32S3:3232
✅ WakeWordDetector ready
KWS fs=16000Hz frames=65 mfcc=10 mel=40 classes=3 idx=0 thr=0.300
```

---

## 🔄 OTA Updates

* Config in `env.h`:

  ```cpp
  #define OTA_HOSTNAME "Marvin-4-ESP32S3"
  #define OTA_PASSWORD "marvinOTA2025"
  #define OTA_PORT 3232
  ```
* Switch to OTA uploads:

  ```ini
  ; platformio.ini (example)
  upload_protocol = espota
  upload_port     = Marvin-4-ESP32S3.local
  upload_flags    = --auth=marvinOTA2025
  ```
* Upload:

  ```bash
  pio run -t upload
  ```

---

## 📡 Runtime Behavior

* **Main loop** reads I2S, builds MFCC window (65×10), runs DSCNN, smooths confidence, and when `avg > WAKE_PROB_THRESH`:

  * **Beeps** the buzzer (GPIO 41 via NPN)
  * Optionally toggles LED or triggers further logic (extend in `main.cpp`)
* **AHT10** polled once per second (ok to unplug; firmware keeps working)

---

## 🧪 Troubleshooting

**“WARNING: Zero input to model” / `PCM_RMS=0.0`**

* Check **mic wiring** exactly:

  * INMP441 **L/R → GND**
  * `BCLK=4`, `LRCL=5`, `DOUT=6`
  * 3V3 and GND solid; **no level shifters**
* Confirm I2S driver started (you should see  “✅ Audio path ready” or equivalent).
* Ensure `AudioCapture::readFrame(...)` returns true (serial will show a one-shot I2S read demo on first boot in some builds; keep if helpful).
* Some boards ship with **different I2S pin mux** defaults. If needed, edit `include/env.h` pins and re-flash.

**Model outputs nonsense / never triggers**

* Verify **labels order** in `labels.h` matches training export.
* Ensure `WAKE_CLASS_INDEX` in `env.h` points at the wake label (index `0` if “marvin” is first).
* Re-check `mfcc_norm.h` constants `MFCC_NORM_SCALE` and `MFCC_NORM_OFFSET` match your export.

**AHT10 warnings**

* Benign; the library logs ambiguous overload warnings on ESP32 Arduino’s `Wire`.
* If sensor is missing or wiring is off, you’ll see “❌ AHT10 not detected”.

**Buzzer silent**

* Confirm **GPIO 41** toggles HIGH on wake (serial prints “BEEP”).
* Verify NPN orientation, base resistor, and buzzer wiring (buzzer to +3V3, transistor to GND).
* Try a quick test in `setup()`:

  ```cpp
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
  ```

---

## 🛠️ Development Notes

* Project uses **tabs for indentation**.
* Keep generated / exported artifacts minimal and in predictable places:

  * **Model weights:** `models/model_weights_float.h`
  * **Frontend constants:** `include/frontend_params.h`
  * **Labels:** `include/labels.h`
* Avoid duplicate macro definitions; prefer the single source of truth in the above headers.

---

## 📅 Work Log

| Date       | Work Done                                    | Version |
| ---------- | -------------------------------------------- | ------- |
| 2025-09-18 | Project scaffolding, audio path & MFCC stack | 0.1.0   |
| 2025-09-19 | DSCNN + labels + OTA + AHT10                 | 0.2.0   |
| 2025-09-19 | I2S zero-input diagnostics & buzzer driver   | 0.3.0   |

---

## 📜 License

MIT. See `LICENSE`.

---

**Maintained by**: Michael Zampogna — JetNetAI