#include "AudioCapture.h"

bool AudioCapture::probe_() {
	// First, ensure I2S is not already installed
	i2s_driver_uninstall(I2S_NUM_0);
	delay(100);

	i2s_config_t cfg = {};
	cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
	cfg.sample_rate = KWS_SAMPLE_RATE_HZ;
	cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
	cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
	cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
	cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
	cfg.dma_buf_count = 8;
	cfg.dma_buf_len = 256;
	cfg.use_apll = false;
	cfg.tx_desc_auto_clear = false;
	cfg.fixed_mclk = 0;

	esp_err_t e = i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
	if (e != ESP_OK) {
		Serial.printf("ERROR: I2S driver install failed: %s\n", esp_err_to_name(e));
		return false;
	}

	i2s_pin_config_t pins = {};
	pins.mck_io_num = I2S_PIN_NO_CHANGE;
	pins.bck_io_num = I2S_BCLK_PIN;
	pins.ws_io_num = I2S_LRCL_PIN;
	pins.data_out_num = I2S_PIN_NO_CHANGE;
	pins.data_in_num = I2S_DOUT_PIN;
	e = i2s_set_pin(I2S_NUM_0, &pins);
	if (e != ESP_OK) {
		Serial.printf("ERROR: I2S pin config failed: %s\n", esp_err_to_name(e));
		return false;
	}

	// Make sure clock is exactly what we want
	e = i2s_set_clk(I2S_NUM_0, KWS_SAMPLE_RATE_HZ, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
	if (e != ESP_OK) {
		Serial.printf("ERROR: I2S clock config failed: %s\n", esp_err_to_name(e));
		return false;
	}

	Serial.printf("INFO: I2S configured - SR=%d, pins: BCLK=%d, WS=%d, DIN=%d\n",
		KWS_SAMPLE_RATE_HZ, I2S_BCLK_PIN, I2S_LRCL_PIN, I2S_DOUT_PIN);

	return true;
}

bool AudioCapture::begin() {
	return probe_();
}

bool AudioCapture::readFrame(int16_t* pcm_out) {
	const size_t need_bytes = AP_FRAME_SAMPLES * sizeof(int32_t);
	size_t got = 0;
	int32_t raw32[AP_FRAME_SAMPLES];

	// Block until full frame
	esp_err_t e = i2s_read(I2S_NUM_0, raw32, need_bytes, &got, portMAX_DELAY);
	if (e != ESP_OK || got < need_bytes) {
		Serial.printf("ERROR: I2S read failed: %s, got %d/%d bytes\n", 
			esp_err_to_name(e), got, need_bytes);
		memset(pcm_out, 0, AP_FRAME_SAMPLES * sizeof(int16_t));
		return false;
	}

	// Convert 24-bit MSB-aligned in 32-bit container -> int16_t
	for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
		// INMP441 is 24-bit left-justified; shift down ~8–12 bits
		int32_t s = raw32[i] >> 11;	// try 11; adjust if too loud/quiet
		if (s > 32767) s = 32767;
		if (s < -32768) s = -32768;
		pcm_out[i] = (int16_t)s;
	}

	return true;
}


