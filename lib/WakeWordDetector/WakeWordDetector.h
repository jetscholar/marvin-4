#ifndef WAKEWORDDETECTOR_H
#define WAKEWORDDETECTOR_H

#include <Arduino.h>
#include "AudioCapture.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"
#include "RingBuffer.h"

class WakeWordDetector {
public:
    WakeWordDetector(AudioCapture& audio);
    bool detect();
    void start();
    void stop();

private:
    AudioCapture& audio;
    AudioProcessor processor;
    ManualDSCNN dscnn;
    RingBuffer<int16_t> buffer;
    bool isRunning;
    void processBuffer();
};

#endif