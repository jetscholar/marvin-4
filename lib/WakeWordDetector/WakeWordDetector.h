#pragma once
#include <Arduino.h>
#include "frontend_params.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"

// Adjust if your labels.txt places the wake class elsewhere.
#ifndef WAKE_CLASS_INDEX
#define WAKE_CLASS_INDEX 0
#endif

#ifndef WAKE_PROB_THRESH
#define WAKE_PROB_THRESH 0.34f
#endif

class EMA {
public:
	explicit EMA(float a = 0.1f) : alpha(a), inited(false), value(0.f) {}
	inline void reset() { inited = false; value = 0.f; }
	inline void add(float x) {
		if (!inited) { value = x; inited = true; }
		else { value = alpha * x + (1.f - alpha) * value; }
	}
	inline float avg() const { return value; }
private:
	float alpha;
	bool  inited;
	float value;
};

class WakeWordDetector {
public:
	WakeWordDetector(AudioProcessor& proc) : processor(proc) {}

	bool init() { ema.reset(); return model.begin(); }

	// Runs one inference. Returns true if smoothed prob crosses threshold.
	bool detect_once(float& p_out, float& avg_out);

	inline float getAverageConfidence() const { return ema.avg(); }
	inline float getThreshold() const { return WAKE_PROB_THRESH; }

private:
	AudioProcessor& processor;
	ManualDSCNN     model;
	EMA             ema { 0.08f };
	uint32_t        lastDumpMs = 0;
};







