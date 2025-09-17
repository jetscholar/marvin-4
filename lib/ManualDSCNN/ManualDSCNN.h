#pragma once
#include <Arduino.h>
#include "frontend_params.h"   // KWS_FRAMES, KWS_NUM_MFCC, KWS_NUM_CLASSES

class ManualDSCNN {
public:
	ManualDSCNN() = default;
	bool begin();  // no-op, kept for API symmetry

	// Full forward pass: mfcc_flat shape = KWS_FRAMES * KWS_NUM_MFCC
	void predict_full(const float* mfcc_flat, float* probs);

	// Convenience: returns probability of the wake class (index 0 by default)
	float predict_proba(const float* mfcc_flat);

private:
	// --- primitive ops ---
	void softmax_(const float* z, float* out, int n) const;

	void conv2d_3x3_(const float* in, int H, int W, int Cin,
	                 const float* k, int Cout,
	                 float* out, bool same) const;

	void bn_hwcn_(float* x, int H, int W, int C,
	              const float* gamma, const float* beta,
	              const float* mean,  const float* var,
	              float eps) const;

	void conv1x1_(const float* in, int H, int W, int Cin,
	              const float* k, int Cout,
	              float* out) const;

	void avgpool2x2_(const float* in, int H, int W, int C,
	                 float* out) const;

	void global_avgpool_(const float* in, int H, int W, int C,
	                     float* out) const;

	void dense_(const float* x, int N,
                const float* W, const float* b, int M,
                float* y) const;

	static inline float relu(float x) { return x > 0.f ? x : 0.f; }
};





