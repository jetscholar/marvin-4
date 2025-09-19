#pragma once
#include <stdint.h>
#include "frontend_params.h"

class ManualDSCNN {
public:
	bool begin();

	// Convenience overload (fills only probs)
	void predict_full(const float* mfcc_flat, float* probs);

	// Full variant (fills probs and optionally logits if non-null)
	void predict_full(const float* mfcc_flat, float* probs, float* logits);

	float predict_proba(const float* mfcc_flat);

private:
	// ---- math/primitives ----
	inline float relu(float x) const { return x > 0 ? x : 0; }
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
};






