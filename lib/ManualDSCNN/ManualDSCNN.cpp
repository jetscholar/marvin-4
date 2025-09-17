#include "ManualDSCNN.h"
#include <string.h>
#include <algorithm>

// Prefer header from /models at project root, with a robust fallback.
// Add -Imodels to build_flags to take the first path.
#if __has_include("model_weights_float.h")
  #include "model_weights_float.h"
#elif __has_include("../../models/model_weights_float.h")
  #include "../../models/model_weights_float.h"
#else
  #error "model_weights_float.h not found. Put it in /models and add -Imodels to build_flags, or adjust the include path."
#endif

bool ManualDSCNN::begin() { return true; }

void ManualDSCNN::predict_full(const float* mfcc_flat, float* probs) {
	const int H0 = KWS_FRAMES;     // e.g., 65
	const int W0 = KWS_NUM_MFCC;   // e.g., 10
	const int C0 = 1;

	// 1) 3x3 conv SAME, Cin=1 → Cout=16
	static float b1[65 * 10 * 16]; // sized for worst-case; compile-time constants ok
	conv2d_3x3_(mfcc_flat, H0, W0, C0, conv2d_1_w, 16, b1, true);
	bn_hwcn_(b1, H0, W0, 16,
	         batch_normalization_7_gamma,
	         batch_normalization_7_beta,
	         batch_normalization_7_mean,
	         batch_normalization_7_var, 1e-5f);
	for (int i = 0; i < H0*W0*16; ++i) b1[i] = relu(b1[i]);

	// 2) AvgPool 2x2
	const int H1 = (H0 + 1) / 2;
	const int W1 = (W0 + 1) / 2;
	static float p1[(65/2 + 1) * (10/2 + 1) * 16]; // small slack
	avgpool2x2_(b1, H0, W0, 16, p1);

	// 3) 1x1 PW: 16→24 + BN + ReLU
	const int C1 = 24;
	static float pw1[(65/2 + 1) * (10/2 + 1) * 24];
	conv1x1_(p1, H1, W1, 16, b1_pw_w, C1, pw1);
	bn_hwcn_(pw1, H1, W1, C1,
	         batch_normalization_9_gamma,
	         batch_normalization_9_beta,
	         batch_normalization_9_mean,
	         batch_normalization_9_var, 1e-5f);
	for (int i = 0; i < H1*W1*C1; ++i) pw1[i] = relu(pw1[i]);

	// 4) AvgPool 2x2
	const int H2 = (H1 + 1) / 2;
	const int W2 = (W1 + 1) / 2;
	static float p2[(65/4 + 1) * (10/4 + 1) * 24];
	avgpool2x2_(pw1, H1, W1, C1, p2);

	// 5) 1x1 PW: 24→32 + BN + ReLU
	const int C2 = 32;
	static float pw2[(65/4 + 1) * (10/4 + 1) * 32];
	conv1x1_(p2, H2, W2, C1, b2_pw_w, C2, pw2);
	bn_hwcn_(pw2, H2, W2, C2,
	         batch_normalization_11_gamma,
	         batch_normalization_11_beta,
	         batch_normalization_11_mean,
	         batch_normalization_11_var, 1e-5f);
	for (int i = 0; i < H2*W2*C2; ++i) pw2[i] = relu(pw2[i]);

	// 6) AvgPool 2x2
	const int H3 = (H2 + 1) / 2;
	const int W3 = (W2 + 1) / 2;
	static float p3[(65/8 + 1) * (10/8 + 1) * 32];
	avgpool2x2_(pw2, H2, W2, C2, p3);

	// 7) 1x1 PW: 32→48 + BN + ReLU
	const int C3 = 48;
	static float pw3[(65/8 + 1) * (10/8 + 1) * 48];
	conv1x1_(p3, H3, W3, C2, b3_pw_w, C3, pw3);
	bn_hwcn_(pw3, H3, W3, C3,
	         batch_normalization_13_gamma,
	         batch_normalization_13_beta,
	         batch_normalization_13_mean,
	         batch_normalization_13_var, 1e-5f);
	for (int i = 0; i < H3*W3*C3; ++i) pw3[i] = relu(pw3[i]);

	// 8) Global average pooling → [C3]
	float gap[48];
	global_avgpool_(pw3, H3, W3, C3, gap);

	// 9) Dense → logits
	float logits[KWS_NUM_CLASSES];
	dense_(gap, C3, dense_1_w, dense_1_b, KWS_NUM_CLASSES, logits);

	// 10) Softmax → probs
	softmax_(logits, probs, KWS_NUM_CLASSES);
}

float ManualDSCNN::predict_proba(const float* mfcc_flat) {
	float probs[KWS_NUM_CLASSES];
	predict_full(mfcc_flat, probs);
	// If your labels.txt has "marvin" at index 0, this returns the wake prob.
	return probs[0];
}

// ====== Primitive ops ======
void ManualDSCNN::softmax_(const float* z, float* out, int n) const {
	float m = z[0];
	for (int i = 1; i < n; ++i) m = std::max(m, z[i]);
	float sum = 0.0f;
	for (int i = 0; i < n; ++i) { out[i] = expf(z[i] - m); sum += out[i]; }
	const float inv = 1.0f / (sum + 1e-9f);
	for (int i = 0; i < n; ++i) out[i] *= inv;
}

void ManualDSCNN::conv2d_3x3_(const float* in, int H, int W, int Cin,
                              const float* k, int Cout,
                              float* out, bool same) const {
	(void)same; // SAME via bounds checks
	const int out_sz = H * W * Cout;
	for (int i = 0; i < out_sz; ++i) out[i] = 0.0f;

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			for (int co = 0; co < Cout; ++co) {
				float acc = 0.0f;
				for (int ci = 0; ci < Cin; ++ci) {
					for (int ky = -1; ky <= 1; ++ky) {
						for (int kx = -1; kx <= 1; ++kx) {
							const int iy = y + ky, ix = x + kx;
							if (iy < 0 || ix < 0 || iy >= H || ix >= W) continue;
							const int wi = ((ky + 1) * 3 + (kx + 1)) * Cin * Cout + ci * Cout + co;
							const int ii = (iy * W + ix) * Cin + ci;
							acc += in[ii] * k[wi];
						}
					}
				}
				out[(y * W + x) * Cout + co] = acc;
			}
		}
	}
}

void ManualDSCNN::bn_hwcn_(float* x, int H, int W, int C,
                           const float* gamma, const float* beta,
                           const float* mean,  const float* var,
                           float eps) const {
	const int HW = H * W;
	for (int hw = 0; hw < HW; ++hw) {
		float* row = &x[hw * C];
		for (int c = 0; c < C; ++c) {
			row[c] = gamma[c] * ((row[c] - mean[c]) / sqrtf(var[c] + eps)) + beta[c];
		}
	}
}

void ManualDSCNN::conv1x1_(const float* in, int H, int W, int Cin,
                           const float* k, int Cout,
                           float* out) const {
	const int HW = H * W;
	for (int hw = 0; hw < HW; ++hw) {
		const float* vin = &in[hw * Cin];
		float* vout = &out[hw * Cout];
		for (int co = 0; co < Cout; ++co) {
			float acc = 0.0f;
			for (int ci = 0; ci < Cin; ++ci) {
				acc += vin[ci] * k[ci * Cout + co];
			}
			vout[co] = acc;
		}
	}
}

void ManualDSCNN::avgpool2x2_(const float* in, int H, int W, int C,
                              float* out) const {
	const int Ho = (H + 1) / 2;
	const int Wo = (W + 1) / 2;
	for (int y = 0; y < Ho; ++y) {
		for (int x = 0; x < Wo; ++x) {
			for (int c = 0; c < C; ++c) {
				float s = 0.0f; int cnt = 0;
				for (int dy = 0; dy < 2; ++dy) {
					for (int dx = 0; dx < 2; ++dx) {
						const int iy = y * 2 + dy, ix = x * 2 + dx;
						if (iy < H && ix < W) { s += in[(iy * W + ix) * C + c]; cnt++; }
					}
				}
				out[(y * Wo + x) * C + c] = s / (float)cnt;
			}
		}
	}
}

void ManualDSCNN::global_avgpool_(const float* in, int H, int W, int C,
                                  float* out) const {
	const float inv = 1.0f / (float)(H * W);
	for (int c = 0; c < C; ++c) {
		float s = 0.0f;
		for (int y = 0; y < H; ++y)
			for (int x = 0; x < W; ++x)
				s += in[(y * W + x) * C + c];
		out[c] = s * inv;
	}
}

void ManualDSCNN::dense_(const float* x, int N,
                         const float* W, const float* b, int M,
                         float* y) const {
	for (int m = 0; m < M; ++m) {
		float acc = b[m];
		for (int n = 0; n < N; ++n) acc += x[n] * W[n * M + m];
		y[m] = acc;
	}
}



