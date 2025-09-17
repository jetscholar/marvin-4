#include "AudioProcessor.h"

// ===== Public =====

AudioProcessor::AudioProcessor()
: fft_(real_, imag_, AP_FFT_SIZE, /*samplingFrequency=*/KWS_SAMPLE_RATE_HZ) {
    memset(real_, 0, sizeof(real_));
    memset(imag_, 0, sizeof(imag_));
    memset(mfcc_ring_, 0, sizeof(mfcc_ring_));
}

bool AudioProcessor::begin() {
    buildMel_();
    buildDct_();
    ring_head_ = 0;
    ring_full_ = false;
    return true;
}

bool AudioProcessor::processFrame(const int16_t* pcm) {
    float mfcc_row[KWS_NUM_MFCC];
    computeMfcc_(pcm, mfcc_row);

    // push into ring
    memcpy(mfcc_ring_[ring_head_], mfcc_row, sizeof(mfcc_row));
    ring_head_ = (ring_head_ + 1) % KWS_FRAMES;
    if (ring_head_ == 0) ring_full_ = true;
    return true;
}

void AudioProcessor::computeMFCCFloat(float* out_flat) const {
    // Flatten rolling window oldest..newest
    const int frames_ready = ring_full_ ? KWS_FRAMES : ring_head_;
    const int total = KWS_FRAMES * KWS_NUM_MFCC;

    // If not yet full, zero-pad the oldest portion
    int out_i = 0;
    if (!ring_full_) {
        const int pad_frames = KWS_FRAMES - frames_ready;
        const int pad_vals = pad_frames * KWS_NUM_MFCC;
        memset(out_flat, 0, sizeof(float) * pad_vals);
        out_i += pad_vals;
    }

    // Oldest frame index
    int oldest = ring_full_ ? ring_head_ : 0;
    for (int i = 0; i < frames_ready; ++i) {
        int idx = (oldest + i) % KWS_FRAMES;
        memcpy(&out_flat[out_i], mfcc_ring_[idx], sizeof(float) * KWS_NUM_MFCC);
        out_i += KWS_NUM_MFCC;
    }

    // Safety: if anything left, zero it (shouldn’t happen)
    if (out_i < total) {
        memset(&out_flat[out_i], 0, sizeof(float) * (total - out_i));
    }
}

// ===== Private: builders =====

void AudioProcessor::buildMel_() {
    // Triangular filterbank on FFT bins
    // Mel range
    const float f_min = 0.0f;
    const float f_max = KWS_SAMPLE_RATE_HZ * 0.5f;

    const float mel_min = hz2mel_(f_min);
    const float mel_max = hz2mel_(f_max);

    // KWS_NUM_MEL + 2 edges
    float mel_edges[KWS_NUM_MEL + 2];
    for (int i = 0; i < KWS_NUM_MEL + 2; ++i) {
        float mel = mel_min + (mel_max - mel_min) * i / (KWS_NUM_MEL + 1);
        mel_edges[i] = mel;
    }

    // Precompute center freqs in Hz for bin mapping
    float hz_edges[KWS_NUM_MEL + 2];
    for (int i = 0; i < KWS_NUM_MEL + 2; ++i) {
        hz_edges[i] = mel2hz_(mel_edges[i]);
    }

    // FFT bin frequencies
    float bin_hz[AP_FFT_BINS];
    for (int b = 0; b < AP_FFT_BINS; ++b) {
        bin_hz[b] = (float)b * KWS_SAMPLE_RATE_HZ / AP_FFT_SIZE;
    }

    // Build triangular weights
    memset(mel_w_, 0, sizeof(mel_w_));
    for (int m = 0; m < KWS_NUM_MEL; ++m) {
        const float f_left  = hz_edges[m];
        const float f_center= hz_edges[m+1];
        const float f_right = hz_edges[m+2];

        for (int b = 0; b < AP_FFT_BINS; ++b) {
            const float f = bin_hz[b];
            float w = 0.0f;
            if (f >= f_left && f <= f_center) {
                w = (f - f_left) / (f_center - f_left + 1e-12f);
            } else if (f > f_center && f <= f_right) {
                w = (f_right - f) / (f_right - f_center + 1e-12f);
            }
            mel_w_[m][b] = w;
        }
    }
    mel_ready_ = true;
}

void AudioProcessor::buildDct_() {
    // DCT-II basis (orthogonal-ish, no lifter)
    const float norm0 = sqrtf(1.0f / KWS_NUM_MEL);
    const float norm  = sqrtf(2.0f / KWS_NUM_MEL);
    for (int i = 0; i < KWS_NUM_MFCC; ++i) {
        for (int j = 0; j < KWS_NUM_MEL; ++j) {
            const float w = (float)M_PI / KWS_NUM_MEL * (j + 0.5f) * i;
            dct_[i][j] = (i == 0 ? norm0 : norm) * cosf(w);
        }
    }
    dct_ready_ = true;
}

// ===== Private: one-row MFCC =====

void AudioProcessor::computeMfcc_(const int16_t* pcm, float* mfcc_row) {
    // 1) Window to float buffer with zero padding to AP_FFT_SIZE
    //    Use Hann window
    const int N = AP_FRAME_SAMPLES;
    for (int i = 0; i < AP_FFT_SIZE; ++i) {
        if (i < N) {
            float w = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (N - 1)));
            real_[i] = (float)pcm[i] * w;
        } else {
            real_[i] = 0.0f;
        }
        imag_[i] = 0.0f;
    }

    // 2) FFT
    fft_.windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD); // already applied Hann; use rect here
    fft_.compute(FFT_FORWARD);
    fft_.complexToMagnitude();

    // 3) Power spectrum (use first AP_FFT_BINS)
    //    real_ now holds magnitudes; compute power
    // 4) Mel filterbank
    float mel_energies[KWS_NUM_MEL];
    for (int m = 0; m < KWS_NUM_MEL; ++m) {
        float e = 0.0f;
        for (int b = 0; b < AP_FFT_BINS; ++b) {
            const float mag = real_[b];
            const float pwr = mag * mag;
            e += pwr * mel_w_[m][b];
        }
        // 5) log
        mel_energies[m] = logf(e + 1e-10f);
    }

    // 6) DCT → MFCC_NUM
    for (int c = 0; c < KWS_NUM_MFCC; ++c) {
        float acc = 0.0f;
        for (int m = 0; m < KWS_NUM_MEL; ++m) {
            acc += dct_[c][m] * mel_energies[m];
        }
        mfcc_row[c] = acc;
    }

    // 7) Per-coefficient normalization (match training)
    for (int c = 0; c < KWS_NUM_MFCC; ++c) {
        mfcc_row[c] = (mfcc_row[c] - MFCC_MEAN[c]) / MFCC_STD[c];
    }
}








