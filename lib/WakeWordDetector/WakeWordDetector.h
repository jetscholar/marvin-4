#ifndef WAKEWORDDETECTOR_H
#define WAKEWORDDETECTOR_H

#include <Arduino.h>
#include "frontend_params.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"

class WakeWordDetector {
public:
	// Pass a reference to the AudioProcessor used in main loop
	WakeWordDetector(AudioProcessor* ap);
	bool init();

	// Runs one detection step; returns current marvin probability
	float detect();

	// Smoothed probability (EMA)
	float getAverageConfidence() const { return avg_; }

	// Threshold from frontend_params.h
	float getThreshold() const { return KWS_TRIGGER_THRESHOLD; }

private:
	AudioProcessor* ap_ = nullptr;
	ManualDSCNN model_;
	float avg_ = 0.0f;
	const float alpha_ = 0.2f;	// smoothing factor
};

#endif






