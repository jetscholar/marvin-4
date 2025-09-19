#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include "env.h"
#include "frontend_params.h"

class AudioCapture {
public:
	bool begin();
	bool readFrame(int16_t* pcm_out);	// fills AP_FRAME_SAMPLES

private:
	bool probe_();
};




