#include "AudioProcessor.h"

// Small epsilon to avoid log(0)
static constexpr float kEps = 1e-10f;

static inline float hz_to_mel_(float hz) {
  return 2595.0f * log10f(1.0f + hz / 700.0f);
}
static inline float mel_to_hz_(float mel) {
  return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

AudioProcessor::AudioProcessor()
: fft_(real_, imag_, FFT_SIZE, SAMPLE_RATE) {
  // Zero buffers
  for (int i = 0; i < FFT_SIZE; i++) { real_[i] = 0.0f; imag_[i] = 0.0f; }
  for (int f = 0; f < MFCC_NUM_FRAMES; f++) {
    for (int c = 0; c < MFCC_NUM_COEFFS; c++) mfcc_ring_[f][c] = 0.0f;
  }
  buildMelBanks_();
  buildDct_();
}

void AudioProcessor::buildMelBanks_() {
  // Nyquist band
  const float f_min = 0.0f;
  const float f_max = SAMPLE_RATE * 0.5f;

  const float mel_min = hz_to_mel_(f_min);
  const float mel_max = hz_to_mel_(f_max);
  const float mel_step = (mel_max - mel_min) / (MEL_FILTER_BANKS + 1);

  float f_centers[MEL_FILTER_BANKS + 2];
  for (int m = 0; m < MEL_FILTER_BANKS + 2; m++) {
    f_centers[m] = mel_to_hz_(mel_min + mel_step * m);
  }

  int bins[MEL_FILTER_BANKS + 2];
  for (int m = 0; m < MEL_FILTER_BANKS + 2; m++) {
    float bin = (f_centers[m] * FFT_SIZE) / SAMPLE_RATE;
    int ib = (int)roundf(bin);
    if (ib < 0) ib = 0;
    if (ib >= K_FFT_BINS) ib = K_FFT_BINS - 1;
    bins[m] = ib;
  }

  for (int m = 0; m < MEL_FILTER_BANKS; m++) {
    for (int k = 0; k < K_FFT_BINS; k++) mel_weights_[m][k] = 0.0f;

    int left = bins[m], center = bins[m + 1], right = bins[m + 2];

    for (int k = left; k <= center; k++) {
      mel_weights_[m][k] = (center != left) ? (float)(k - left) / (float)(center - left) : 0.0f;
    }
    for (int k = center; k <= right; k++) {
      float w = (right != center) ? (float)(right - k) / (float)(right - center) : 0.0f;
      if (w > mel_weights_[m][k]) mel_weights_[m][k] = w;
    }
  }
  banks_built_ = true;
}

void AudioProcessor::buildDct_() {
  const float M = (float)MEL_FILTER_BANKS;
  const float norm = sqrtf(2.0f / M);

  for (int n = 0; n < MFCC_NUM_COEFFS; n++) {
    for (int k = 0; k < MEL_FILTER_BANKS; k++) {
      float val = norm * cosf((float)M_PI / M * ((float)k + 0.5f) * (float)n);
      if (n == 0) val *= M_SQRT1_2;  // DC scaling
      dct_matrix_[n][k] = val;
    }
  }
}

void AudioProcessor::runFft_() {
  // real_ is assumed pre-filled; imag_ zeroed
  // Note: We already window in computeMfcc_. The lines below keep a consistent call sequence.
  fft_.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft_.compute(FFT_FORWARD);
  fft_.complexToMagnitude();
}

void AudioProcessor::computeMfcc_(int16_t* pcm, float* dst_coeffs) {
  // 1) Pre-emphasis + Hamming; zero-pad to FFT_SIZE
  for (int i = 0; i < WINDOW_SIZE; i++) {
    float x = (float)pcm[i];
    float prev = (i > 0) ? (float)pcm[i - 1] : 0.0f;
    float y = x - PRE_EMPHASIS * prev;
    float w = 0.54f - 0.46f * cosf((2.0f * (float)M_PI * i) / (WINDOW_SIZE - 1));
    real_[i] = y * w;
    imag_[i] = 0.0f;
  }
  for (int i = WINDOW_SIZE; i < FFT_SIZE; i++) { real_[i] = 0.0f; imag_[i] = 0.0f; }

  // 2) FFT magnitude -> power
  runFft_();
  static float power[K_FFT_BINS];
  for (int k = 0; k < K_FFT_BINS; k++) {
    float mag = real_[k];
    power[k] = mag * mag;
  }

  // 3) Mel filterbank log-energies
  float melE[MEL_FILTER_BANKS];
  for (int m = 0; m < MEL_FILTER_BANKS; m++) {
    float e = 0.0f;
    for (int k = 0; k < K_FFT_BINS; k++) e += mel_weights_[m][k] * power[k];
    melE[m] = logf(e + kEps);
  }

  // 4) DCT -> MFCCs
  for (int c = 0; c < MFCC_NUM_COEFFS; c++) {
    float sum = 0.0f;
    for (int m = 0; m < MEL_FILTER_BANKS; m++) sum += dct_matrix_[c][m] * melE[m];
    dst_coeffs[c] = sum;
  }
}

void AudioProcessor::processFrame(int16_t* frame) {
  float mfcc_row[MFCC_NUM_COEFFS];
  computeMfcc_(frame, mfcc_row);

  // push into ring (overwrite oldest when full)
  for (int c = 0; c < MFCC_NUM_COEFFS; c++) {
    mfcc_ring_[ring_head_][c] = mfcc_row[c];
  }
  ring_head_ = (ring_head_ + 1) % ring_size_;
  if (ring_head_ == 0) ring_filled_ = true;
}

void AudioProcessor::copyMfccWindowFloat(float* out_flat) const {
  // Emit in chronological order: oldest -> newest
  const int frames = ring_size_;
  const int start = ring_filled_ ? ring_head_ : 0;
  const int count = ring_filled_ ? frames : ring_head_;

  // If not yet full, we still output 'frames' rows; the newest part will be zeros.
  int out_idx = 0;

  if (ring_filled_) {
    // from start to end-1
    for (int f = 0; f < frames; f++) {
      int idx = (start + f) % frames;
      for (int c = 0; c < MFCC_NUM_COEFFS; c++) out_flat[out_idx++] = mfcc_ring_[idx][c];
    }
  } else {
    // pad leading with zeros (oldest missing frames)
    for (int f = 0; f < frames - count; f++) {
      for (int c = 0; c < MFCC_NUM_COEFFS; c++) out_flat[out_idx++] = 0.0f;
    }
    // then copy available frames in order 0..ring_head_-1
    for (int f = 0; f < count; f++) {
      for (int c = 0; c < MFCC_NUM_COEFFS; c++) out_flat[out_idx++] = mfcc_ring_[f][c];
    }
  }
}

void AudioProcessor::getLastMfcc(float* out_row) const {
  int last = (ring_head_ - 1 + ring_size_) % ring_size_;
  for (int c = 0; c < MFCC_NUM_COEFFS; c++) out_row[c] = mfcc_ring_[last][c];
}




