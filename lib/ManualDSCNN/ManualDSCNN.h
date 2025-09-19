#ifndef MANUALDSCNN_H
#define MANUALDSCNN_H

#include <stdint.h>
#include "frontend_params.h"

class ManualDSCNN {
public:
	ManualDSCNN();
	bool begin();
	void predict_full(const float* mfcc_flat, float* probs, float* logits = nullptr);
	float predict_proba(const float* mfcc_flat);

private:
	// Conv1: Depthwise 3x3x1x16
	float conv1_weights[3][3][1][16];
	float conv1_gamma[16];  // batch_normalization_7_gamma
	float conv1_beta[16];   // batch_normalization_7_beta
	float conv1_mean[16];   // batch_normalization_7_mean
	float conv1_var[16];    // batch_normalization_7_var
	float conv1_gamma_post[16];  // batch_normalization_8_gamma
	float conv1_beta_post[16];   // batch_normalization_8_beta
	float conv1_mean_post[16];   // batch_normalization_8_mean
	float conv1_var_post[16];    // batch_normalization_8_var
	// Conv2: Pointwise 1x1x16x24
	float conv2_weights[1][1][16][24];
	float conv2_gamma[24];  // batch_normalization_9_gamma
	float conv2_beta[24];   // batch_normalization_9_beta
	float conv2_mean[24];   // batch_normalization_9_mean
	float conv2_var[24];    // batch_normalization_9_var
	float conv2_gamma_post[24];  // batch_normalization_10_gamma
	float conv2_beta_post[24];   // batch_normalization_10_beta
	float conv2_mean_post[24];   // batch_normalization_10_mean
	float conv2_var_post[24];    // batch_normalization_10_var
	// Dense (24 inputs to 3 classes)
	float dense_weights[24][3];  // Stubbed dense_1_w
	float dense_bias[3];         // dense_1_b
};

#endif



