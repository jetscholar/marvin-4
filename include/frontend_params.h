#pragma once
// ==== Model/frontend export (edit these to match your model export) ====

#define KWS_SAMPLE_RATE_HZ   16000
#define KWS_FRAME_MS         20     // analysis window (ms)
#define KWS_STRIDE_MS        10     // hop (ms)
#define KWS_NUM_MEL          40
#define KWS_NUM_MFCC         10
#define KWS_FRAMES           65     // time steps
#define KWS_NUM_CLASSES      3

// Derived frontend constants used by the pipeline.
// Keep these as macros so they are usable in array sizes.
#define AP_FRAME_SAMPLES   ((KWS_SAMPLE_RATE_HZ * KWS_FRAME_MS)  / 1000)   // e.g., 320 for 20ms @ 16k
#define AP_HOP_SAMPLES     ((KWS_SAMPLE_RATE_HZ * KWS_STRIDE_MS) / 1000)   // e.g., 160 for 10ms @ 16k

// FFT sizing for MFCC (power-of-two >= AP_FRAME_SAMPLES)
#define AP_FFT_SIZE        512
#define AP_FFT_BINS        (AP_FFT_SIZE/2 + 1)

// Optional: index of the wake class if you want it centralized here
// #define KWS_LABEL_MARVIN_IDX 0
