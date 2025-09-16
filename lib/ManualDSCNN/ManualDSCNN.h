#ifndef MANUAL_DSCNN_H
#define MANUAL_DSCNN_H

#include <Arduino.h>
#include "frontend_params.h"	// KWS_FRAMES, KWS_NUM_MFCC, KWS_NUM_CLASSES

class ManualDSCNN {
public:
	ManualDSCNN() = default;
	bool begin();	// no-op (kept for symmetry)

	// Return probability (0..1) of "marvin" (index 0).
	float predict_proba(const float* mfcc_flat);

	// Fill probs[ KWS_NUM_CLASSES ] with softmax outputs.
	void predict_full(const float* mfcc_flat, float* probs);

private:
	// Activations / utils
	inline float relu(float x) const { return x > 0.0f ? x : 0.0f; }
	void softmax_(const float* logits, float* out, int n) const;

	// Layers
	void conv2d_3x3_(const float* in, int H, int W, int Cin,
	                 const float* k, int Cout,
	                 float* out, bool same=true) const;

	// Proper per-channel BatchNorm on [H,W,C] tensor (NHWC)
	void bn_hwcn_(float* x, int H, int W, int C,
	              const float* gamma, const float* beta,
	              const float* mean,  const float* var,
	              float eps=1e-5f) const;

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
};

#endif



