#include "WakeWordDetector.h"
#include "frontend_params.h"
#include <Arduino.h>

bool WakeWordDetector::begin() {
	Serial.println("DEBUG: WakeWordDetector begin");
	return true;
}

bool WakeWordDetector::detect_once(float& p_conf, float& p_avg) {
	Serial.println("DEBUG: detect_once started");
	int16_t pcm[AP_FRAME_SAMPLES];
	if (!cap_.readFrame(pcm)) {
		Serial.println("DEBUG: readFrame failed");
		p_conf = 0.0f;
		p_avg = p_avg_;
		Serial.flush();
		return false;
	}
	Serial.println("DEBUG: readFrame succeeded");

	proc_.processFrame(pcm);
	Serial.println("DEBUG: processFrame called");

	float mfcc[KWS_FRAMES * KWS_NUM_MFCC];
	proc_.computeMFCCFloat(mfcc);
	Serial.println("DEBUG: computeMFCCFloat called");

	p_conf = net_.predict_proba(mfcc);
	Serial.printf("DEBUG: predict_proba returned p_conf=%.4f\n", p_conf);

	p_avg_ = 0.9f * p_avg_ + 0.1f * p_conf;
	p_avg = p_avg_;

	bool detected = p_conf >= WAKE_PROB_THRESH;
	if (detected) {
		Serial.println("DEBUG: Wake word detected!");
	}
	Serial.printf("DEBUG: detect_once done, p_conf=%.4f, p_avg=%.4f\n", p_conf, p_avg);
	Serial.flush();
	return detected;
}










