#pragma once
#include <stdint.h>
#include "frontend_params.h"

class AudioProcessor {
public:
	AudioProcessor();

	bool begin();
	void processFrame(const int16_t* pcm_frame);		// push 20ms
	void computeMFCCFloat(float* out_mfcc_flat);		// KWS_FRAMES*KWS_NUM_MFCC

	// Debug/introspection used by detector
	bool hasFullWindow() const { return ring_full_; }
	float lastPcmRms() const { return last_pcm_rms_; }
	float lastMfccMeanAbs() const { return last_mfcc_mean_abs_; }

private:
	// ring buffer of frames
	int16_t	ring_[KWS_FRAMES][AP_FRAME_SAMPLES];
	int		ring_head_;
	bool	ring_full_;

	// cached MFCC window (flattened)
	float	mfcc_[KWS_FRAMES * KWS_NUM_MFCC];
	float	last_pcm_rms_;
	float	last_mfcc_mean_abs_;

	// mel & dct
	float	mel_[KWS_NUM_MEL * (AP_FRAME_SAMPLES/2 + 1)];
	float	dct_[KWS_NUM_MFCC * KWS_NUM_MEL];

	void	buildMel_();
	void	buildDct_();
	void	computeMfcc_(const int16_t* pcm, float* mfcc_row);
};















