#include "WakeWordDetector.h"

#include "WakeWordDetector.h"

WakeWordDetector::WakeWordDetector(AudioCapture& audio) : audio(audio), buffer(AUDIO_BUFFER_SIZE) {
    isRunning = false;
}

bool WakeWordDetector::detect() {
    if (!isRunning) return false;

    processBuffer();
    if (buffer.available() >= WINDOW_SIZE) {
        int16_t frame[WINDOW_SIZE];
        buffer.read(frame, WINDOW_SIZE);
        float mfcc[MFCC_NUM_COEFFS * MFCC_NUM_FRAMES];
        processor.processFrame(frame, mfcc);
        float score = dscnn.infer(mfcc);
        if (score > WAKE_WORD_THRESHOLD) {
            Serial.println("✅ Wake word detected!");
            return true;
        }
    }
    return false;
}

void WakeWordDetector::start() {
    isRunning = true;
    buffer.reset();
}

void WakeWordDetector::stop() {
    isRunning = false;
}

void WakeWordDetector::processBuffer() {
    while (audio.readSample() != 0) {
        buffer.write(audio.readSample());
    }
}