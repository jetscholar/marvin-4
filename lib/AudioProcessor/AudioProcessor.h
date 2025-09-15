#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>
#include <ArduinoFFT.h>
#include <math.h>
#include "env.h"

// WINDOW_SIZE (frame length) must be <= FFT_SIZE
static_assert(WINDOW_SIZE <= FFT_SIZE, "WINDOW_SIZE must be <= FFT_SIZE");

// kFftBins = DC..Nyquist
static constexpr int K_FFT_BINS = (FFT_SIZE / 2) + 1;

class AudioProcessor {
public:
  AudioProcessor();

  // Feed one PCM frame of WINDOW_SIZE samples (int16)
  // Internally: pre-emphasis -> window -> FFT -> log-mel -> DCT (MFCC)
  // Then it pushes one MFCC row [MFCC_NUM_COEFFS] into a rolling buffer
  void processFrame(int16_t* frame);

  // Copy the current rolling MFCC window as float, flattened to [frames * coeffs]
  // Layout: oldest ... newest (what most KWS models expect)
  void copyMfccWindowFloat(float* out_flat) const;

  // (Debug helper) get last-written MFCC row
  void getLastMfcc(float* out_row) const;

private:
  // Persistent FFT buffers (avoid stack blowups)
  ArduinoFFT<float> fft_;
  float real_[FFT_SIZE];
  float imag_[FFT_SIZE];

  // Mel & DCT caches
  bool banks_built_ = false;
  float mel_weights_[MEL_FILTER_BANKS][K_FFT_BINS];       // triangular filterbank weights
  float dct_matrix_[MFCC_NUM_COEFFS][MEL_FILTER_BANKS];   // DCT-II

  // Rolling MFCC window buffer [MFCC_NUM_FRAMES x MFCC_NUM_COEFFS]
  float mfcc_ring_[MFCC_NUM_FRAMES][MFCC_NUM_COEFFS];
  int   ring_size_ = MFCC_NUM_FRAMES;
  int   ring_head_ = 0;  // next write index
  bool  ring_filled_ = false;

  // Steps
  void buildMelBanks_();
  void buildDct_();
  void runFft_(); // uses real_/imag_ in-place

  // One-frame MFCC extraction into dst[MFCC_NUM_COEFFS]
  void computeMfcc_(int16_t* pcm, float* dst_coeffs);
};

#endif






