#ifndef MFCC_NORM_H
#define MFCC_NORM_H

// Per-coefficient normalization for on-device MFCCs
#define MFCC_NUM_COEFFS 10

const float MFCC_MEAN[10] = { -5.27866554e+01f, 6.02387953e+00f, 3.31878781e-01f, -8.33101943e-02f, -6.50086939e-01f, -3.90249103e-01f, -3.79662037e-01f, -3.41269195e-01f, -5.88298321e-01f, -3.49105597e-01f };
const float MFCC_STD[10] = { 3.71451836e+01f, 7.29888344e+00f, 5.25638199e+00f, 3.89305401e+00f, 3.51031899e+00f, 2.69939542e+00f, 2.35839033e+00f, 2.06454110e+00f, 1.97835672e+00f, 1.72913826e+00f };

// Simple normalization constants for AudioProcessor compatibility
#define MFCC_NORM_SCALE 1.0f
#define MFCC_NORM_OFFSET 0.0f

#endif