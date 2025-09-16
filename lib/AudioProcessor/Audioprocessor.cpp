#include "AudioProcessor.h"

static constexpr float kPreEmph = 0.97f;
static constexpr float kEps = 1e-10f;

AudioProcessor::AudioProcessor()
: fft_(real_, imag_, AP_FFT_SIZE, KWS_SAMPLE_RATE_HZ) {
	for (int f = 0; f < KWS_FRAMES; ++f)
		for (int c = 0; c < KWS_NUM_MFCC; ++c)
			mfcc_ring_[f][c] = 0.0f;
	for (int i = 0; i < AP_FFT_SIZE; ++i) { real_[i] = 0.0f; imag_[i] = 0.0f; }
}

bool AudioProcessor::begin() {
	buildMel_();
	buildDct_();
	return true;
}

void AudioProcessor::buildMel_() {
	const float f_min = 0.0f;
	const float f_max = 0.5f * (float)KWS_SAMPLE_RATE_HZ;

	const float mel_min = hz2mel_(f_min);
	const float mel_max = hz2mel_(f_max);
	const float mel_step = (mel_max - mel_min) / (KWS_NUM_MEL + 1);

	float centers_hz[KWS_NUM_MEL + 2];
	for (int m = 0; m < KWS_NUM_MEL + 2; ++m) {
		centers_hz[m] = mel2hz_(mel_min + mel_step * m);
	}

	int bins[KWS_NUM_MEL + 2];
	for (int m = 0; m < KWS_NUM_MEL + 2; ++m) {
		float binf = (centers_hz[m] * AP_FFT_SIZE) / (float)KWS_SAMPLE_RATE_HZ;
		int b = (int)roundf(binf);
		if (b < 0) b = 0;
		if (b >= AP_FFT_BINS) b = AP_FFT_BINS - 1;
		bins[m] = b;
	}

	for (int m = 0; m < KWS_NUM_MEL; ++m) {
		for (int k = 0; k < AP_FFT_BINS; ++k) mel_w_[m][k] = 0.0f;
		const int l = bins[m], c = bins[m + 1], r = bins[m + 2];

		for (int k = l; k <= c; ++k) {
			mel_w_[m][k] = (c != l) ? (float)(k - l) / (float)(c - l) : 0.0f;
		}
		for (int k = c; k <= r; ++k) {
			float w = (r != c) ? (float)(r - k) / (float)(r - c) : 0.0f;
			if (w > mel_w_[m][k]) mel_w_[m][k] = w;
		}
	}
	mel_ready_ = true;
}

void AudioProcessor::buildDct_() {
	const float M = (float)KWS_NUM_MEL;
	const float norm = sqrtf(2.0f / M);

	for (int n = 0; n < KWS_NUM_MFCC; ++n) {
		for (int k = 0; k < KWS_NUM_MEL; ++k) {
			float v = norm * cosf((float)M_PI / M * ((float)k + 0.5f) * (float)n);
			if (n == 0) v *= M_SQRT1_2;	// DC scale
			dct_[n][k] = v;
		}
	}
	dct_ready_ = true;
}

bool AudioProcessor::processFrame(const int16_t* pcm) {
	if (!mel_ready_ || !dct_ready_) return false;

	float mfcc[KWS_NUM_MFCC];
	computeMfcc_(pcm, mfcc);

	// Push into ring (oldest overwritten)
	for (int c = 0; c < KWS_NUM_MFCC; ++c) {
		mfcc_ring_[ring_head_][c] = mfcc[c];
	}
	ring_head_ = (ring_head_ + 1) % KWS_FRAMES;
	if (ring_head_ == 0) ring_full_ = true;
	return true;
}

void AudioProcessor::computeMfcc_(const int16_t* pcm, float* mfcc_row) {
	// 1) Pre-emphasis + Hamming + zero-pad
	for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
		float x = (float)pcm[i];
		float px = (i > 0) ? (float)pcm[i - 1] : 0.0f;
		float y = x - kPreEmph * px;
		float w = 0.54f - 0.46f * cosf((2.0f * (float)M_PI * i) / (AP_FRAME_SAMPLES - 1));
		real_[i] = y * w;
		imag_[i] = 0.0f;
	}
	for (int i = AP_FRAME_SAMPLES; i < AP_FFT_SIZE; ++i) {
		real_[i] = 0.0f; imag_[i] = 0.0f;
	}

	static float power[AP_FFT_BINS];
	for (int k = 0; k < AP_FFT_BINS; ++k) {
		float mag = real_[k];
		power[k] = mag * mag;
	}

    // 2) FFT → magnitude^2 power spectrum
    fft_.compute(FFT_FORWARD);
    fft_.complexToMagnitude();

	// 3) Mel energies (log)
	float mel[KWS_NUM_MEL];
	for (int m = 0; m < KWS_NUM_MEL; ++m) {
		float e = 0.0f;
		for (int k = 0; k < AP_FFT_BINS; ++k) e += mel_w_[m][k] * power[k];
		mel[m] = logf(e + kEps);
	}

	// 4) DCT → MFCC (take first KWS_NUM_MFCC)
	for (int c = 0; c < KWS_NUM_MFCC; ++c) {
		float s = 0.0f;
		for (int m = 0; m < KWS_NUM_MEL; ++m) s += dct_[c][m] * mel[m];
		// 5) Per-coeff normalization (match training)
		float z = (s - MFCC_MEAN[c]) / MFCC_STD[c];
		mfcc_row[c] = z;
	}
}

void AudioProcessor::copyMfccWindowFloat(float* out_flat) const {
	// Emit oldest → newest; pad leading zeros if not yet full
	const int count = ring_full_ ? KWS_FRAMES : ring_head_;
	const int pad = KWS_FRAMES - count;

	int idx = 0;
	// pad (oldest missing)
	for (int f = 0; f < pad; ++f) {
        for (int c = 0; c < KWS_NUM_MFCC; ++c) out_flat[idx++] = 0.0f;
	}
	// existing frames in chronological order
	if (ring_full_) {
		int start = ring_head_;
		for (int f = 0; f < KWS_FRAMES; ++f) {
			int r = (start + f) % KWS_FRAMES;
			for (int c = 0; c < KWS_NUM_MFCC; ++c) out_flat[idx++] = mfcc_ring_[r][c];
		}
	} else {
		for (int f = 0; f < count; ++f) {
			for (int c = 0; c < KWS_NUM_MFCC; ++c) out_flat[idx++] = mfcc_ring_[f][c];
		}
	}
}







