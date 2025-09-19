#include "WakeWordDetector.h"
#include <Arduino.h>
#include "labels.h"

bool WakeWordDetector::begin() {
	return cap_.begin() && proc_.begin() && net_.begin();
}

bool WakeWordDetector::detect_once(float& p_conf, float& p_avg) {
	int16_t pcm[AP_FRAME_SAMPLES];

	// 1) Capture a frame
	if (!cap_.readFrame(pcm)) {
		p_conf = 0.0f; p_avg = conf_avg_;
		Serial.println(F("WARNING: Zero input to model"));
		return false;
	}

	// 2) Push into processor
	proc_.processFrame(pcm);

	// 3) Need full window first
	if (!proc_.hasFullWindow()) {
		p_conf = 0.0f; p_avg = conf_avg_;
		return false;
	}

	// 4) Compute MFCCs (flattened)
	static float mfcc[KWS_FRAMES * KWS_NUM_MFCC];
	proc_.computeMFCCFloat(mfcc);

	// 5) Run model
	float probs[KWS_NUM_CLASSES];
	net_.predict_full(mfcc, probs);

	p_conf = probs[WAKE_CLASS_INDEX];
	// smooth avg
	conf_avg_ = 0.8f * conf_avg_ + 0.2f * p_conf;
	p_avg = conf_avg_;

	// Optional debug
	if (DEBUG_LEVEL >= 3) {
		Serial.printf("PCM_RMS=%.1f MFCC|mean_abs=%.4f p=%.3f avg=%.3f\n",
			proc_.lastPcmRms(), proc_.lastMfccMeanAbs(), p_conf, p_avg);
	}

	// 6) Threshold
	return (p_conf >= WAKE_PROB_THRESH);
}











