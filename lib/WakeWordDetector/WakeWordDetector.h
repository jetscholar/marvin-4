#ifndef WAKEWORDDETECTOR_H
#define WAKEWORDDETECTOR_H

#include <Arduino.h>
#include "env.h"

class AudioProcessor; // fwd declare

class WakeWordDetector {
public:
  WakeWordDetector(AudioProcessor* proc);
  bool init();
  float detect();  // runs inference and returns "marvin" confidence [0..1]

  // Accessors
  bool  isInitialized() const { return initialized_; }
  float getThreshold()  const { return threshold_; }
  unsigned long getLastDetectionTime() const { return last_detection_ms_; }
  int   getDetectionCount() const { return detection_count_; }
  float getAverageConfidence() const;

private:
  AudioProcessor* ap_ = nullptr;
  bool initialized_ = false;

  // runtime threshold/cooldown from env.h
  float threshold_ = WAKE_WORD_THRESHOLD;
  unsigned long last_detection_ms_ = 0;
  int detection_count_ = 0;

  // confidence averaging
  static constexpr int CONF_HISTORY = 20;
  float conf_hist_[CONF_HISTORY];
  int   conf_idx_ = 0;
  int   conf_count_ = 0;

  // TFLM stuff
  static constexpr int kTensorArenaSize = MODEL_ARENA_SIZE;
  alignas(16) static uint8_t tensor_arena_[kTensorArenaSize];

  const struct tfliteModelTag* model_ = nullptr; // opaque pointer type forward (see .cpp)
  class tfliteMicroInterpreterTag* interpreter_ = nullptr;
  struct TfLiteTensorTag* input_ = nullptr;
  struct TfLiteTensorTag* output_ = nullptr;

  // cached quantization for input tensor
  float   in_scale_ = 1.0f;
  int32_t in_zero_  = 0;

  // helper
  void pushConf_(float c);
};

#endif



