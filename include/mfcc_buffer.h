#ifndef MFCC_BUFFER_H
#define MFCC_BUFFER_H

#include "env.h"
#include <Arduino.h>

// Global buffer for MFCC features (flattened [frames * coeffs])
extern int8_t mfcc_buffer[MFCC_NUM_FRAMES * MFCC_NUM_COEFFS];

#endif
