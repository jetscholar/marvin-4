#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>
#include "env.h"
#include "RingBuffer.h"

class AudioProcessor {
public:
    AudioProcessor();
    bool processFrame(int16_t* samples, float* mfcc_out);
    void reset();

private:
    float melFilterBank[MEL_FILTER_BANKS][WINDOW_SIZE];
    void computeFFT(int16_t* samples, float* fft_out);
    void applyMelFilter(float* fft, float* mel_out);
    void dct(float* mel, float* mfcc_out);
};

#endif