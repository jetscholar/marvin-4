#include "ManualDSCNN.h"

ManualDSCNN::ManualDSCNN() {
    loadModel();
}

float ManualDSCNN::infer(float* mfcc_input) {
    float sum = 0.0;
    for (int i = 0; i < MFCC_NUM_COEFFS * MFCC_NUM_FRAMES; i++) {
        sum += mfcc_input[i] * weights[i % MODEL_ARENA_SIZE];
    }
    return 1.0 / (1.0 + expf(-sum)); // Sigmoid activation
}

void ManualDSCNN::loadModel() {
    for (int i = 0; i < MODEL_ARENA_SIZE; i++) {
        weights[i] = 0.1; // Dummy weights, replace with trained model
    }
}