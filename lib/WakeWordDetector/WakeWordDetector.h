#pragma once
#include "env.h"
#include "frontend_params.h"
#include "AudioCapture.h"
#include "AudioProcessor.h"
#include "ManualDSCNN.h"

class WakeWordDetector {
public:
	WakeWordDetector(AudioCapture& cap, AudioProcessor& proc, ManualDSCNN& net)
	: cap_(cap), proc_(proc), net_(net), conf_avg_(0.0f) {}

	bool begin();
	bool detect_once(float& p_conf, float& p_avg);

private:
	AudioCapture&	cap_;
	AudioProcessor&	proc_;
	ManualDSCNN&	net_;
	float			conf_avg_;
};











