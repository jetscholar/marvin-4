#include "AudioProcessor.h"
#include "arduinoFFT.h" // Corrected include to match library header

AudioProcessor::AudioProcessor() {
    for (int i = 0; i < MEL_FILTER_BANKS; i++) {
        for (int j = 0; j < WINDOW_SIZE; j++) {
            melFilterBank[i][j] = 0.0; // Placeholder, replace with actual coefficients
        }
    }
}

bool AudioProcessor::processFrame(int16_t* samples, float* mfcc_out) {
    if (!samples || !mfcc_out) return false;

    float fft_out[FFT_SIZE];
    computeFFT(samples, fft_out);
    float mel_out[MEL_FILTER_BANKS];
    applyMelFilter(fft_out, mel_out);
    dct(mel_out, mfcc_out);
    return true;
}

void AudioProcessor::reset() {
    // Reset state if needed
}

void AudioProcessor::computeFFT(int16_t* samples, float* fft_out) {
    ArduinoFFT FFT = ArduinoFFT(samples, fft_out, FFT_SIZE, SAMPLE_RATE); // Changed to ArduinoFFT
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Changed to FFT_WIN_TYP_HAMMING
    FFT.Compute(FFT_FORWARD);
    FFT.ComplexToMagnitude();
}

void AudioProcessor::applyMelFilter(float* fft, float* mel_out) {
    for (int i = 0; i < MEL_FILTER_BANKS; i++) {
        mel_out[i] = 0.0;
        for (int j = 0; j < WINDOW_SIZE/2; j++) {
            mel_out[i] += fft[j] * melFilterBank[i][j];
        }
        mel_out[i] = log10f(mel_out[i] + 1e-6); // Log energy
    }
}

void AudioProcessor::dct(float* mel, float* mfcc_out) {
    for (int i = 0; i < MFCC_NUM_COEFFS; i++) {
        mfcc_out[i] = 0.0;
        for (int j = 0; j < MEL_FILTER_BANKS; j++) {
            float angle = PI * i * (j + 0.5) / MEL_FILTER_BANKS;
            mfcc_out[i] += mel[j] * cosf(angle);
        }
        mfcc_out[i] *= sqrt(2.0 / MEL_FILTER_BANKS);
    }
}