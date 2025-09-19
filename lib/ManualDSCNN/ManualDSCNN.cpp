#include "ManualDSCNN.h"
#include <Arduino.h>
#include <string.h>
#include <math.h>
#include "frontend_params.h"
#include "model_weights_float.h"

ManualDSCNN::ManualDSCNN() {
	// Load weights and biases from model_weights_float.h
	memcpy(conv1_weights, conv2d_1_w, sizeof(conv2d_1_w));  // 3x3x1x16
	memcpy(conv1_gamma, batch_normalization_7_gamma, sizeof(batch_normalization_7_gamma));
	memcpy(conv1_beta, batch_normalization_7_beta, sizeof(batch_normalization_7_beta));
	memcpy(conv1_mean, batch_normalization_7_mean, sizeof(batch_normalization_7_mean));
	memcpy(conv1_var, batch_normalization_7_var, sizeof(batch_normalization_7_var));
	memcpy(conv1_gamma_post, batch_normalization_8_gamma, sizeof(batch_normalization_8_gamma));
	memcpy(conv1_beta_post, batch_normalization_8_beta, sizeof(batch_normalization_8_beta));
	memcpy(conv1_mean_post, batch_normalization_8_mean, sizeof(batch_normalization_8_mean));
	memcpy(conv1_var_post, batch_normalization_8_var, sizeof(batch_normalization_8_var));
	memcpy(conv2_weights, b1_pw_w, sizeof(b1_pw_w));  // 1x1x16x24
	memcpy(conv2_gamma, batch_normalization_9_gamma, sizeof(batch_normalization_9_gamma));
	memcpy(conv2_beta, batch_normalization_9_beta, sizeof(batch_normalization_9_beta));
	memcpy(conv2_mean, batch_normalization_9_mean, sizeof(batch_normalization_9_mean));
	memcpy(conv2_var, batch_normalization_9_var, sizeof(batch_normalization_9_var));
	memcpy(conv2_gamma_post, batch_normalization_10_gamma, sizeof(batch_normalization_10_gamma));
	memcpy(conv2_beta_post, batch_normalization_10_beta, sizeof(batch_normalization_10_beta));
	memcpy(conv2_mean_post, batch_normalization_10_mean, sizeof(batch_normalization_10_mean));
	memcpy(conv2_var_post, batch_normalization_10_var, sizeof(batch_normalization_10_var));
	// Stub dense_weights with zeros (24x3, adjust when full dense_1_w available)
	memset(dense_weights, 0, sizeof(float) * 24 * KWS_NUM_CLASSES);
	memcpy(dense_bias, dense_1_b, sizeof(dense_1_b));  // 3
}

bool ManualDSCNN::begin() {
	Serial.println("DEBUG: ManualDSCNN begin");
	Serial.println("DEBUG: ManualDSCNN weights loaded from model_weights_float.h (dense_1_w stubbed)");
	Serial.flush();
	return true;
}

void ManualDSCNN::predict_full(const float* mfcc_flat, float* probs, float* logits) {
	if (!mfcc_flat || !probs) {
		Serial.println("ERROR: Invalid input to predict_full");
		return;
	}
	if (!logits) {
		logits = new float[KWS_NUM_CLASSES]();
	}
	memset(logits, 0, KWS_NUM_CLASSES * sizeof(float));
	memset(probs, 0, KWS_NUM_CLASSES * sizeof(float));

	// Reshape MFCC to 2D input [KWS_FRAMES][KWS_NUM_MFCC]
	float input[KWS_FRAMES][KWS_NUM_MFCC];
	for (int t = 0; t < KWS_FRAMES; ++t) {
		for (int c = 0; c < KWS_NUM_MFCC; ++c) {
			input[t][c] = mfcc_flat[t * KWS_NUM_MFCC + c];
		}
	}

	// Block 1: Depthwise Conv 3x3 (1 in, 16 out)
	float* conv1_out = new float[KWS_FRAMES * KWS_NUM_MFCC * 16]();  // Heap alloc
	for (int t = 0; t < KWS_FRAMES; ++t) {
		for (int f = 0; f < KWS_NUM_MFCC; ++f) {
			for (int oc = 0; oc < 16; ++oc) {
				float sum = 0;
				for (int kt = -1; kt <= 1; ++kt) {
					for (int kf = -1; kf <= 1; ++kf) {
						int it = t + kt, ic = f + kf;
						if (it >= 0 && it < KWS_FRAMES && ic >= 0 && ic < KWS_NUM_MFCC) {
							sum += input[it][ic] * conv1_weights[kt + 1][kf + 1][0][oc];
						}
					}
				}
				conv1_out[(t * KWS_NUM_MFCC + f) * 16 + oc] = (sum > 0) ? sum : 0;  // ReLU
			}
		}
	}
	// BatchNorm for Conv1
	for (int t = 0; t < KWS_FRAMES; ++t) for (int f = 0; f < KWS_NUM_MFCC; ++f)
		for (int c = 0; c < 16; ++c) {
			float x = conv1_out[(t * KWS_NUM_MFCC + f) * 16 + c];
			float norm = (x - conv1_mean[c]) / sqrtf(conv1_var[c] + 1e-5f);
			conv1_out[(t * KWS_NUM_MFCC + f) * 16 + c] = norm * conv1_gamma[c] + conv1_beta[c];
		}

	// Pointwise Conv 1x1 (16 in, 24 out)
	float* conv2_out = new float[KWS_FRAMES * KWS_NUM_MFCC * 24]();  // Heap alloc
	for (int t = 0; t < KWS_FRAMES; ++t) {
		for (int f = 0; f < KWS_NUM_MFCC; ++f) {
			for (int oc = 0; oc < 24; ++oc) {
				float sum = 0;
				for (int ic = 0; ic < 16; ++ic) {
					sum += conv1_out[(t * KWS_NUM_MFCC + f) * 16 + ic] * conv2_weights[0][0][ic][oc];
				}
				conv2_out[(t * KWS_NUM_MFCC + f) * 24 + oc] = (sum > 0) ? sum : 0;  // ReLU
			}
		}
	}
	// BatchNorm for Conv2
	for (int t = 0; t < KWS_FRAMES; ++t) for (int f = 0; f < KWS_NUM_MFCC; ++f)
		for (int c = 0; c < 24; ++c) {
			float x = conv2_out[(t * KWS_NUM_MFCC + f) * 24 + c];
			float norm = (x - conv2_mean[c]) / sqrtf(conv2_var[c] + 1e-5f);
			conv2_out[(t * KWS_NUM_MFCC + f) * 24 + c] = norm * conv2_gamma[c] + conv2_beta[c];
		}

	// Global Average Pooling
	float gap[24] = {0};
	for (int oc = 0; oc < 24; ++oc) {
		for (int t = 0; t < KWS_FRAMES; ++t) {
			for (int f = 0; f < KWS_NUM_MFCC; ++f) {
				gap[oc] += conv2_out[(t * KWS_NUM_MFCC + f) * 24 + oc];
			}
		}
		gap[oc] /= (KWS_FRAMES * KWS_NUM_MFCC);
	}

	// Dense Layer (24 inputs to 3 classes)
	for (int oc = 0; oc < KWS_NUM_CLASSES; ++oc) {
		float sum = dense_bias[oc];
		for (int ic = 0; ic < 24; ++ic) {
			sum += gap[ic] * dense_weights[ic][oc];
		}
		logits[oc] = sum;
		if (isnan(logits[oc]) || isinf(logits[oc])) logits[oc] = 0.0f;
	}

	// Softmax
	float max_logit = logits[0];
	for (int i = 1; i < KWS_NUM_CLASSES; ++i) {
		if (logits[i] > max_logit) max_logit = logits[i];
	}
	float sum_exp = 0.0f;
	for (int i = 0; i < KWS_NUM_CLASSES; ++i) {
		probs[i] = expf(logits[i] - max_logit);
		if (isnan(probs[i]) || isinf(probs[i])) probs[i] = 0.0f;
		sum_exp += probs[i];
	}
	for (int i = 0; i < KWS_NUM_CLASSES; ++i) {
		probs[i] /= (sum_exp > 0 ? sum_exp : 1.0f);
	}

	delete[] conv1_out;
	delete[] conv2_out;

	if (logits != probs) delete[] logits;
}

float ManualDSCNN::predict_proba(const float* mfcc_flat) {
	float probs[KWS_NUM_CLASSES];
	predict_full(mfcc_flat, probs, nullptr);
	for (int i = 0; i < KWS_NUM_CLASSES; ++i) {
		if (isnan(probs[i]) || isinf(probs[i])) probs[i] = 0.0f;
	}
	float max_p = 0.0f;
	for (int i = 0; i < KWS_NUM_CLASSES; ++i) {
		if (probs[i] > max_p) max_p = probs[i];
	}
	return max_p;
}