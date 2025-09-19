#ifndef WAKEWORDDETECTOR_H
#define WAKEWORDDETECTOR_H

#include "AudioCapture.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"

class WakeWordDetector {
public:
  WakeWordDetector(AudioCapture& cap, AudioProcessor& proc, ManualDSCNN& net)
    : cap_(cap), proc_(proc), net_(net) {}
  bool begin();
  bool detect_once(float& p_conf, float& p_avg);

private:
  AudioCapture& cap_;
  AudioProcessor& proc_;
  ManualDSCNN& net_;
  float p_avg_ = 0.0f;
};

#endif











