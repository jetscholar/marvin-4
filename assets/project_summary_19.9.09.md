Marvin-4 KWS Project Summary (as of September 19, 2025, 8:13 PM AEST)
Overview
This conversation has focused on debugging and improving the Marvin-4 Keyword Spotting (KWS) system on an ESP32-S3, aimed at detecting the wake word "marvin" using an INMP441 microphone, with additional AHT10 sensor integration for temperature and humidity. The system uses I2S for audio capture, ArduinoFFT for MFCC computation, and a custom ManualDSCNN model for wake word detection. The project has faced persistent issues with audio input, FFT computation, and AHT10 communication, detailed below.
Progress and Issues

Initial State (September 19, 2025, 3:00 PM AEST):

Problem: Compilation errors in AudioCapture.cpp (syntax error at line 92) and AudioProcessor.cpp (invalid sum += mel_ in computeMfcc_, missing braces).
Action: Fixed syntax errors, updated AudioCapture.cpp and AudioProcessor.cpp with correct brace closure and computeMfcc_ logic. Confirmed wiring: INMP441 (VDD=3.3V, GND=GND, L/R=GND, SCK=4, WS=5, SD=6), AHT10 (SDA=8, SCL=9, 4.7kΩ pull-ups), buzzer (GPIO41).
Outcome: Compilation succeeded, but serial output stopped at DEBUG: computeMFCCFloat entered, indicating a crash in AudioProcessor::computeMFCCFloat. AHT10 not detected at 0x38/0x39, despite prior readings (~19.7°C, ~46.8%). PCM input issues persisted (previously 32767, -32768, or 0).


Second Update (8:15 PM AEST):

Problem: Serial stopped at DEBUG: computeMfcc_ FFT start. PCM input zero (PCM samples [0-4]: 0 0 0 0 0), AHT10 detected at 0x38 via I2C scanner but failed to read.
Action: Added granular debug logs, enabled dummy PCM (use_dummy=true), reduced FFT size (AP_FRAME_SAMPLES=512), added heap/stack monitoring, and enhanced I2C reset in main.cpp. Updated AudioCapture.cpp to log raw I2S data.
Outcome: Dummy PCM worked (PCM samples [0-4]: 0 5633 11099 16234 20886, RMS: 23197.5), but real mic input remained zero (Raw I2S [0-4]: 0x00000000). FFT still crashed at ArduinoFFT::compute. AHT10 reads failed despite detection at 0x38.



Current State

Achievements:
Code compiles and runs with detailed debug output up to FFT computation.
Dummy PCM generates valid audio data, confirming pipeline functionality past readFrame.
I2C scanner confirms AHT10 at 0x38.
WiFi and OTA setup work reliably (IP: 172.16.2.231, port: 3232).
Stack usage is low (15KB free), heap sufficient (76KB during FFT).


Issues:
FFT Crash: Pipeline stalls at ArduinoFFT::compute in computeMfcc_, likely due to memory issues, library incompatibility, or floating-point errors.
PCM Input: Real mic input is zero (Raw I2S [0-4]: 0x00000000), suggesting INMP441 or I2S issues (wiring, clock, or bit depth).
AHT10: Detected at 0x38 but AHT10::begin and reads fail, possibly due to weak pull-ups (4.7kΩ) or library issues.
Wake Word: No detection due to FFT crash; stubbed weights should yield p~0.33 if pipeline completes.


Wiring: INMP441 (VDD=3.3V, GND=GND, L/R=GND, SCK=4, WS=5, SD=6), AHT10 (SDA=8, SCL=9, 4.7kΩ pull-ups), LED (GPIO2), buzzer (GPIO41).

Next Steps

FFT Crash: Try ArduinoFFT<int16_t>, update library to 2.0.4, or further reduce AP_FRAME_SAMPLES. Log FFT output (real, imag) for NaN/infinite values.
PCM Input: Test SD=GPIO7, verify mic with multimeter (SD voltage should vary), try I2S_BITS_PER_SAMPLE_16BIT or I2S_CHANNEL_FMT_RIGHT_LEFT.
AHT10: Use 2.2kΩ pull-ups, test manual I2C read (Wire.write(0xAC, 0x33, 0x00)), power cycle AHT10.
Wake Word: Share mfcc_norm.h, model_weights_float.h, ManualDSCNN.cpp to verify model weights and logic.
Debug: Add exception handler, log heap after each major step.
Future: Integrate BH1750 light sensor after wake word detection is stable.

Request
Please share:

New serial log after applying latest AudioCapture.cpp, AudioProcessor.cpp, frontend_params.h.
mfcc_norm.h, model_weights_float.h, ManualDSCNN.cpp.
Results of SD pin voltage test and SD=GPIO7 trial.
Confirmation of 2.2kΩ pull-ups for AHT10.

This summary provides a starting point for further debugging. Let's resolve the FFT and PCM issues to get "marvin" detection working! 🎙️