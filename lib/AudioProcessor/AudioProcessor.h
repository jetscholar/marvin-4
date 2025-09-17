#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <Arduino.h>
#include <ArduinoFFT.h>
#include <math.h>
#include <string.h>
#include "frontend_params.h"   // KWS_* constants
#include "mfcc_norm.h"         // MFCC_MEAN / MFCC_STD

// Sanity checks against generated headers
#ifndef KWS_SAMPLE_RATE_HZ
#error "frontend_params.h must define KWS_SAMPLE_RATE_HZ"
#endif
static_assert(KWS_NUM_MFCC == MFCC_NUM_COEFFS, "MFCC coeff count mismatch to mfcc_norm.h");

// Window & hop (samples) derived from frontend params
// e.g. 30 ms window, 15 ms hop @ 16 kHz => ~480 & ~240 samples
#define AP_FRAME_SAMPLES  ((KWS_SAMPLE_RATE_HZ * KWS_FRAME_MS)   / 1000)
#define AP_HOP_SAMPLES    ((KWS_SAMPLE_RATE_HZ * KWS_STRIDE_MS)  / 1000)

// Use a power-of-two FFT size with zero padding
#define AP_FFT_SIZE       512
static constexpr int AP_FFT_BINS = (AP_FFT_SIZE / 2) + 1;

class AudioProcessor {
public:
    AudioProcessor();

    // One-time init (build mel filters, DCT, clear buffers)
    bool begin();

    // Compute MFCCs for one PCM frame (AP_FRAME_SAMPLES int16) and push into a rolling ring.
    // Call this every STRIDE with new audio.
    bool processFrame(const int16_t* pcm);

    // NEW: Expose the current rolling MFCC window as FLOAT, flattened.
    // Layout: oldest ... newest (time-major), size = KWS_FRAMES * KWS_NUM_MFCC.
    // This is what your model consumes directly.
    void computeMFCCFloat(float* out_flat) const;

    // (Existing helper kept for compatibility.)
    void copyMfccWindowFloat(float* out_flat) const { computeMFCCFloat(out_flat); }

private:
    // FFT (float path)
    ArduinoFFT<float> fft_;
    float real_[AP_FFT_SIZE];
    float imag_[AP_FFT_SIZE];

    // Mel filterbank weights [KWS_NUM_MEL][AP_FFT_BINS]
    bool  mel_ready_ = false;
    float mel_w_[KWS_NUM_MEL][AP_FFT_BINS];

    // DCT-II matrix [KWS_NUM_MFCC][KWS_NUM_MEL]
    bool  dct_ready_ = false;
    float dct_[KWS_NUM_MFCC][KWS_NUM_MEL];

    // Rolling MFCC ring [KWS_FRAMES][KWS_NUM_MFCC]
    float mfcc_ring_[KWS_FRAMES][KWS_NUM_MFCC];
    int   ring_head_ = 0;
    bool  ring_full_ = false;

    // Builders
    void buildMel_();
    void buildDct_();

    // Helpers
    inline float hz2mel_(float hz) const {
        return 2595.0f * log10f(1.0f + hz / 700.0f);
    }
    inline float mel2hz_(float mel) const {
        return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
    }

    // Compute one MFCC row from a PCM frame (windowing → FFT → mel → log → DCT → norm)
    void computeMfcc_(const int16_t* pcm, float* mfcc_row);
};

#endif










