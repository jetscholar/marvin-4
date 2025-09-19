#pragma once
#include "AudioProcessor.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <ArduinoFFT.h>
#include "mfcc_norm.h"
#include "env.h"

// Reduce FFT size for testing
#define AP_FRAME_SAMPLES 512
#define AP_FFT_BINS (AP_FRAME_SAMPLES / 2)

AudioProcessor::AudioProcessor()
    : ring_head_(0), ring_full_(false), last_pcm_rms_(0.0f), last_mfcc_mean_abs_(0.0f) {
    memset(ring_, 0, sizeof(ring_));
    memset(mfcc_, 0, sizeof(mfcc_));
    memset(mel_, 0, sizeof(mel_));
    memset(dct_, 0, sizeof(dct_));
    memset(power_, 0, sizeof(power_));
    for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
        window_[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * (float)i / (float)(AP_FRAME_SAMPLES - 1)));
    }
}

bool AudioProcessor::begin() {
    Serial.println("DEBUG: AudioProcessor begin");
    Serial.printf("DEBUG: Free heap: %u bytes\n", ESP.getFreeHeap());
    buildMel_();
    buildDct_();
    ring_head_ = 0;
    ring_full_ = true;  // Force true for testing
    Serial.flush();
    return true;
}

void AudioProcessor::processFrame(const int16_t* pcm_frame) {
    Serial.println("DEBUG: processFrame entered");
    if (!pcm_frame) {
        Serial.println("ERROR: pcm_frame is null");
        Serial.flush();
        return;
    }
    memcpy(ring_[ring_head_], pcm_frame, sizeof(int16_t) * AP_FRAME_SAMPLES);
    ring_head_ = (ring_head_ + 1) % KWS_FRAMES;
    ring_full_ = (ring_head_ == 0);

    static bool use_dummy = true;
    if (use_dummy) {
        Serial.println("DEBUG: Using dummy PCM");
        for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
            ring_[ring_head_][i] = (int16_t)(32767.0f * sinf(2.0f * M_PI * 440.0f * i / KWS_SAMPLE_RATE_HZ));
        }
    } else {
        float scale = 0.1f;
        for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
            ring_[ring_head_][i] = (int16_t)(pcm_frame[i] * scale);
        }
    }

    long long acc = 0;
    for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
        int32_t s = ring_[ring_head_][i];
        acc += (long long)s * (long long)s;
    }
    last_pcm_rms_ = sqrtf((float)acc / (float)AP_FRAME_SAMPLES);

    static int frame_count = 0;
    if (DEBUG_LEVEL >= 2 && frame_count % 10 == 0) {  // Every 10th frame
        Serial.printf("DEBUG: PCM samples [0-4]: %d %d %d %d %d, RMS: %.1f, ring_full=%d\n",
                      ring_[ring_head_][0], ring_[ring_head_][1], ring_[ring_head_][2],
                      ring_[ring_head_][3], ring_[ring_head_][4], last_pcm_rms_, ring_full_);
        int32_t sum_abs = 0;
        for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
            sum_abs += abs(ring_[ring_head_][i]);
        }
        Serial.printf("DEBUG: PCM sum_abs=%d\n", sum_abs);
        Serial.flush();
    }
    frame_count++;
}

void AudioProcessor::computeMFCCFloat(float* out_mfcc_flat) {
    Serial.println("DEBUG: computeMFCCFloat entered");
    Serial.printf("DEBUG: Free heap: %u bytes\n", ESP.getFreeHeap());
    if (!out_mfcc_flat) {
        Serial.println("ERROR: out_mfcc_flat is null");
        Serial.flush();
        return;
    }
    if (!ring_full_) {
        Serial.println("DEBUG: computeMFCCFloat skipped due to ring_full=false");
        memset(out_mfcc_flat, 0, sizeof(float) * KWS_FRAMES * KWS_NUM_MFCC);
        last_mfcc_mean_abs_ = 0.0f;
        Serial.flush();
        return;
    }

    Serial.println("DEBUG: computeMFCCFloat processing frames");
    float mean_abs = 0.0f;
    int idx = 0;
    int pos = ring_head_;
    static int frame_count = 0;  // Add static frame counter
    for (int f = 0; f < KWS_FRAMES; ++f) {
        const int i = (pos + f) % KWS_FRAMES;
        computeMfcc_(ring_[i], &out_mfcc_flat[idx]);
        idx += KWS_NUM_MFCC;
    }

    Serial.println("DEBUG: computeMFCCFloat normalizing MFCC");
    for (int i = 0; i < KWS_FRAMES * KWS_NUM_MFCC; ++i) {
        int c = i % KWS_NUM_MFCC;
        out_mfcc_flat[i] = (out_mfcc_flat[i] - MFCC_MEAN[c]) / MFCC_STD[c];
        mean_abs += fabsf(out_mfcc_flat[i]);
    }
    last_mfcc_mean_abs_ = mean_abs / (float)(KWS_FRAMES * KWS_NUM_MFCC);

    if (DEBUG_LEVEL >= 2 && frame_count % 10 == 0) {  // Use frame_count, every 10th call
        Serial.printf("DEBUG: MFCC[0-4]: %.4f %.4f %.4f %.4f %.4f, mean_abs: %.4f\n",
                      out_mfcc_flat[0], out_mfcc_flat[1], out_mfcc_flat[2],
                      out_mfcc_flat[3], out_mfcc_flat[4], last_mfcc_mean_abs_);
        Serial.flush();
    }
    frame_count++;  // Increment counter
}

void AudioProcessor::buildMel_() {
    Serial.println("DEBUG: buildMel_ entered");
    const float f_min = 20.0f;
    const float f_max = (float)KWS_SAMPLE_RATE_HZ / 2.0f;
    const float mel_min = 1125.0f * logf(1.0f + f_min / 700.0f);
    const float mel_max = 1125.0f * logf(1.0f + f_max / 700.0f);
    float mel_points[KWS_NUM_MEL + 2];
    for (int i = 0; i < KWS_NUM_MEL + 2; ++i) {
        mel_points[i] = mel_min + (float)i * (mel_max - mel_min) / (float)(KWS_NUM_MEL + 1);
    }
    float freq_points[KWS_NUM_MEL + 2];
    for (int i = 0; i < KWS_NUM_MEL + 2; ++i) {
        freq_points[i] = 700.0f * (expf(mel_points[i] / 1125.0f) - 1.0f);
    }
    int bin_points[KWS_NUM_MEL + 2];
    for (int i = 0; i < KWS_NUM_MEL + 2; ++i) {
        bin_points[i] = (int)((AP_FFT_BINS) * freq_points[i] / (float)KWS_SAMPLE_RATE_HZ);
    }
    memset(mel_, 0, sizeof(mel_));
    for (int m = 0; m < KWS_NUM_MEL; ++m) {
        int lower = bin_points[m];
        int center = bin_points[m + 1];
        int upper = bin_points[m + 2];
        if (center > lower) {
            for (int f = lower; f < center; ++f) {
                if (m * AP_FFT_BINS + f >= KWS_NUM_MEL * AP_FFT_BINS) {
                    Serial.printf("ERROR: Mel index out of bounds: m=%d, f=%d\n", m, f);
                    Serial.flush();
                    return;
                }
                mel_[m * AP_FFT_BINS + f] = (float)(f - lower) / (float)(center - lower);
            }
        }
        if (upper > center) {
            for (int f = center; f < upper; ++f) {
                if (m * AP_FFT_BINS + f >= KWS_NUM_MEL * AP_FFT_BINS) {
                    Serial.printf("ERROR: Mel index out of bounds: m=%d, f=%d\n", m, f);
                    Serial.flush();
                    return;
                }
                mel_[m * AP_FFT_BINS + f] = (float)(upper - f) / (float)(upper - center);
            }
        }
    }
    Serial.println("DEBUG: buildMel_ done");
    Serial.flush();
}

void AudioProcessor::buildDct_() {
    Serial.println("DEBUG: buildDct_ entered");
    const float sqrt_2n = sqrtf(2.0f / (float)KWS_NUM_MEL);
    const float sqrt_1n = sqrtf(1.0f / (float)KWS_NUM_MEL);
    for (int m = 0; m < KWS_NUM_MFCC; ++m) {
        for (int k = 0; k < KWS_NUM_MEL; ++k) {
            if (m * KWS_NUM_MEL + k >= KWS_NUM_MFCC * KWS_NUM_MEL) {
                Serial.printf("ERROR: DCT index out of bounds: m=%d, k=%d\n", m, k);
                Serial.flush();
                return;
            }
            float val = cosf((float)M_PI * (float)m * ((float)k + 0.5f) / (float)KWS_NUM_MEL);
            dct_[m * KWS_NUM_MEL + k] = (m == 0 ? sqrt_1n : sqrt_2n) * val;
        }
    }
    Serial.println("DEBUG: buildDct_ done");
    Serial.flush();
}

void AudioProcessor::computeMfcc_(const int16_t* pcm, float* mfcc_row) {
    Serial.println("DEBUG: computeMfcc_ entered");
    if (!pcm || !mfcc_row) {
        Serial.println("ERROR: pcm or mfcc_row is null");
        Serial.flush();
        return;
    }

    float real[AP_FFT_SIZE];
    float imag[AP_FFT_SIZE];
    memset(real, 0, sizeof(real));
    memset(imag, 0, sizeof(imag));

    for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
        real[i] = (float)pcm[i] * window_[i];
        imag[i] = 0.0f;
    }

    Serial.println("DEBUG: computeMfcc_ FFT start");
    Serial.printf("DEBUG: Free heap before FFT: %u bytes\n", ESP.getFreeHeap());
    ArduinoFFT<float> fft(real, imag, AP_FFT_SIZE, (float)KWS_SAMPLE_RATE_HZ);
    try {
        fft.compute(FFTDirection::Forward);
        fft.complexToMagnitude();
    } catch (...) {
        Serial.println("ERROR: FFT computation failed");
        Serial.flush();
        return;
    }
    Serial.println("DEBUG: computeMfcc_ FFT done");

    for (int i = 0; i < AP_FFT_BINS; ++i) {
        if (i >= AP_FFT_BINS) {
            Serial.printf("ERROR: Power index out of bounds: i=%d\n", i);
            Serial.flush();
            return;
        }
        float mag = real[i] * real[i] + imag[i] * imag[i];
        power_[i] = (mag > 1e20) ? 1e20 : mag;
    }

    static int debug_count = 0;
    if (DEBUG_LEVEL >= 2 && debug_count % 10 == 0) {
        float power_sum = 0.0f;
        for (int i = 0; i < AP_FFT_BINS; ++i) {
            power_sum += power_[i];
        }
        Serial.printf("DEBUG: Power sum=%.4f, first[0-4]=[%.4f %.4f %.4f %.4f %.4f]\n",
                      power_sum, power_[0], power_[1], power_[2], power_[3], power_[4]);
        Serial.flush();
    }

    float mel_energies[KWS_NUM_MEL];
    memset(mel_energies, 0, sizeof(mel_energies));
    for (int m = 0; m < KWS_NUM_MEL; ++m) {
        float energy = 0.0f;
        for (int f = 0; f < AP_FFT_BINS; ++f) {
            energy += power_[f] * mel_[m * AP_FFT_BINS + f];
        }
        mel_energies[m] = (energy > 1e-5f) ? logf(energy) : logf(1e-5f);
    }

    if (DEBUG_LEVEL >= 2 && debug_count % 10 == 0) {
        Serial.printf("DEBUG: Mel energies [0-4]=[%.4f %.4f %.4f %.4f %.4f]\n",
                      mel_energies[0], mel_energies[1], mel_energies[2], mel_energies[3], mel_energies[4]);
        Serial.flush();
    }

    for (int c = 0; c < KWS_NUM_MFCC; ++c) {
        float sum = 0.0f;
        for (int m = 0; m < KWS_NUM_MEL; ++m) {
            sum += mel_energies[m] * dct_[c * KWS_NUM_MEL + m];
        }
        mfcc_row[c] = sum;
    }
    Serial.println("DEBUG: computeMfcc_ done");
    Serial.flush();
    debug_count++;
}





