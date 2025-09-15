#ifndef MANUALDSCNN_H
#define MANUALDSCNN_H

#include <Arduino.h>
#include "env.h"

class ManualDSCNN {
public:
    ManualDSCNN();
    float infer(float* mfcc_input);
    void loadModel();

private:
    float weights[MODEL_ARENA_SIZE]; // Placeholder for model weights
};

#endif