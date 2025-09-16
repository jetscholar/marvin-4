#include "WakeWordDetector.h"

WakeWordDetector::WakeWordDetector(AudioProcessor* ap) : ap_(ap) {}

bool WakeWordDetector::init() {
	return model_.begin();
}

float WakeWordDetector::detect() {
	static float mfcc_flat[KWS_FRAMES * KWS_NUM_MFCC];
	ap_->copyMfccWindowFloat(mfcc_flat);

	const float p = model_.predict_proba(mfcc_flat);
	avg_ = alpha_ * p + (1.0f - alpha_) * avg_;

	if (avg_ >= KWS_TRIGGER_THRESHOLD) {
		// You can guard with a cooldown counter if you want to avoid re-triggers
		// Serial.printf("Wake! p=%.3f avg=%.3f\n", p, avg_);
	}
	return p;
}






