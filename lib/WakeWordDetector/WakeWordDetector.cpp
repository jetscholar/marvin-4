#include "WakeWordDetector.h"

bool WakeWordDetector::detect_once(float& p_out, float& avg_out) {
	// 1) Extract MFCCs (float, normalized like in training)
	static float mfcc_f[KWS_FRAMES * KWS_NUM_MFCC];
	processor.computeMFCCFloat(mfcc_f);

	// 2) Inference
	float probs[KWS_NUM_CLASSES];
	model.predict_full(mfcc_f, probs);

	// 3) Wake probability and smoothing
	const float pWake = probs[WAKE_CLASS_INDEX];
	ema.add(pWake);

	// 4) 1 Hz logging
	const uint32_t now = millis();
	if (now - lastDumpMs > 1000) {
		lastDumpMs = now;
		Serial.printf("p=%.3f avg=%.3f\n", pWake, ema.avg());
	}

	p_out   = pWake;
	avg_out = ema.avg();
	return (ema.avg() > WAKE_PROB_THRESH);
}







