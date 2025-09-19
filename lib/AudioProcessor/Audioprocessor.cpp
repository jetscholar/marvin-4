#include "AudioProcessor.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "mfcc_norm.h"	// defines MFCC_NORM_SCALE, MFCC_NORM_OFFSET

AudioProcessor::AudioProcessor()
: ring_head_(0), ring_full_(false),
  last_pcm_rms_(0.0f), last_mfcc_mean_abs_(0.0f) {
	memset(ring_, 0, sizeof(ring_));
	memset(mfcc_, 0, sizeof(mfcc_));
	memset(mel_, 0, sizeof(mel_));
	memset(dct_, 0, sizeof(dct_));
}

bool AudioProcessor::begin() {
	buildMel_();
	buildDct_();
	ring_head_ = 0;
	ring_full_ = false;
	return true;
}

void AudioProcessor::processFrame(const int16_t* pcm_frame) {
	// copy into ring
	memcpy(ring_[ring_head_], pcm_frame, sizeof(int16_t) * AP_FRAME_SAMPLES);
	ring_head_ = (ring_head_ + 1) % KWS_FRAMES;
	if (ring_head_ == 0) ring_full_ = true;

	// quick RMS
	long long acc = 0;
	for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
		int32_t s = pcm_frame[i];
		acc += (long long)s * (long long)s;
	}
	last_pcm_rms_ = sqrtf((float)acc / (float)AP_FRAME_SAMPLES);

	// Debug: Print some PCM samples every 50 frames
	static int frame_count = 0;
	if (++frame_count % 50 == 0) {
		Serial.printf("DEBUG: PCM samples [0-4]: %d %d %d %d %d, RMS: %.1f\n", 
			pcm_frame[0], pcm_frame[1], pcm_frame[2], pcm_frame[3], pcm_frame[4], last_pcm_rms_);
	}
}

void AudioProcessor::computeMFCCFloat(float* out_mfcc_flat) {
	// compute MFCCs for each frame in ring order oldest..newest
	if (!ring_full_) {
		memset(out_mfcc_flat, 0, sizeof(float) * KWS_FRAMES * KWS_NUM_MFCC);
		last_mfcc_mean_abs_ = 0.0f;
		return;
	}

	float mean_abs = 0.0f;
	int idx = 0;
	int pos = ring_head_;	// oldest
	for (int f = 0; f < KWS_FRAMES; ++f) {
		const int i = (pos + f) % KWS_FRAMES;
		computeMfcc_(ring_[i], &out_mfcc_flat[idx]);
		idx += KWS_NUM_MFCC;
	}

	// optional simple normalization
	for (int i = 0; i < KWS_FRAMES * KWS_NUM_MFCC; ++i) {
		out_mfcc_flat[i] = out_mfcc_flat[i] * MFCC_NORM_SCALE + MFCC_NORM_OFFSET;
		mean_abs += fabsf(out_mfcc_flat[i]);
	}
	last_mfcc_mean_abs_ = mean_abs / (float)(KWS_FRAMES * KWS_NUM_MFCC);

	// Debug: Print first few MFCC values periodically
	static int mfcc_debug_count = 0;
	if (++mfcc_debug_count % 100 == 0) {
		Serial.printf("DEBUG: MFCC[0-4]: %.4f %.4f %.4f %.4f %.4f, mean_abs: %.4f\n",
			out_mfcc_flat[0], out_mfcc_flat[1], out_mfcc_flat[2], out_mfcc_flat[3], out_mfcc_flat[4], last_mfcc_mean_abs_);
	}
}

void AudioProcessor::buildMel_() {
	// TODO: build mel filterbank (placeholder keeps zeros -> model BN should cope)
	// If you have your exported mel weights, fill mel_[m * (NFFT/2+1) + k]
}

void AudioProcessor::buildDct_() {
	// TODO: build DCT-II basis for KWS_NUM_MFCC x KWS_NUM_MEL (placeholder is identity-ish)
	for (int m = 0; m < KWS_NUM_MFCC; ++m) {
		for (int k = 0; k < KWS_NUM_MEL; ++k) {
			dct_[m * KWS_NUM_MEL + k] = (m == k) ? 1.0f : 0.0f;
		}
	}
}

void AudioProcessor::computeMfcc_(const int16_t* pcm, float* mfcc_row) {
	// Enhanced stub: Create more realistic MFCC-like features
	float acc = 0.0f;
	float max_val = 0.0f;
	
	for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
		float val = fabsf((float)pcm[i]);
		acc += val;
		if (val > max_val) max_val = val;
	}
	
	const float mean_energy = acc / (float)AP_FRAME_SAMPLES;
	const float log_energy = (mean_energy > 1.0f) ? logf(mean_energy) : 0.0f;

	// Create more varied coefficients based on energy distribution
	for (int c = 0; c < KWS_NUM_MFCC; ++c) {
		if (c == 0) {
			// First coefficient is log energy
			mfcc_row[c] = log_energy;
		} else {
			// Other coefficients vary with frequency-like patterns
			float freq_weight = sinf(c * 0.5f) * cosf(c * 0.3f);
			mfcc_row[c] = log_energy * freq_weight * (0.1f + 0.01f * c);
		}
	}
}











