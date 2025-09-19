#pragma once
#include "AudioCapture.h"
#include <driver/i2s.h>
#include "frontend_params.h"
#include "env.h"

bool AudioCapture::probe_() {
    i2s_driver_uninstall(I2S_NUM_0);
    delay(100);

    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = KWS_SAMPLE_RATE_HZ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Test this
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = true,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    esp_err_t e = i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
    if (e != ESP_OK) {
        Serial.printf("ERROR: I2S install: %s\n", esp_err_to_name(e));
        Serial.flush();
        return false;
    }

    i2s_pin_config_t pins = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRCL_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DOUT_PIN,
    };

    e = i2s_set_pin(I2S_NUM_0, &pins);
    if (e != ESP_OK) {
        Serial.printf("ERROR: I2S pins: %s\n", esp_err_to_name(e));
        Serial.flush();
        return false;
    }
    Serial.printf("✅ INMP441 I2S init: SR=%d, LEFT, pins: BCLK=%d, WS=%d, DIN=%d\n",
                KWS_SAMPLE_RATE_HZ, I2S_BCLK_PIN, I2S_LRCL_PIN, I2S_DOUT_PIN);
    Serial.flush();

    e = i2s_set_clk(I2S_NUM_0, KWS_SAMPLE_RATE_HZ, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
    if (e != ESP_OK) {
        Serial.printf("ERROR: I2S clk: %s\n", esp_err_to_name(e));
        Serial.flush();
        return false;
    }

    Serial.printf("✅ INMP441 I2S init: SR=%d, LEFT, pins: BCLK=%d, WS=%d, DIN=%d\n",
                  KWS_SAMPLE_RATE_HZ, I2S_BCLK_PIN, I2S_LRCL_PIN, I2S_DOUT_PIN);
    Serial.flush();
    return true;
}

bool AudioCapture::begin() {
    i2s_driver_uninstall(I2S_NUM_0);  // Force uninstall even if error
    delay(100);
    return probe_();
}

bool AudioCapture::readFrame(int16_t* pcm_out) {
    if (!pcm_out) {
        Serial.println("ERROR: pcm_out is null");
        Serial.flush();
        return false;
    }
    const size_t need_bytes = AP_FRAME_SAMPLES * sizeof(int32_t);
    size_t got = 0;
    int32_t raw32[AP_FRAME_SAMPLES];

    esp_err_t e = i2s_read(I2S_NUM_0, raw32, need_bytes, &got, portMAX_DELAY);
    if (e != ESP_OK || got < need_bytes) {
        Serial.printf("ERROR: I2S read: %s, got %d/%d\n", esp_err_to_name(e), got, need_bytes);
        Serial.flush();
        memset(pcm_out, 0, AP_FRAME_SAMPLES * sizeof(int16_t));
        return false;
    }

    static int debug_count = 0;
    bool show_debug = (DEBUG_LEVEL >= 3) && (debug_count % 10 == 0);  // Every 10th frame
    if (show_debug) {
        Serial.printf("DEBUG: Raw I2S bytes [0-4]: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                      raw32[0], raw32[1], raw32[2], raw32[3], raw32[4]);
        int32_t sum_abs = 0;
        for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
            sum_abs += abs(raw32[i]);
        }
        Serial.printf("DEBUG: Raw I2S sum_abs=%d\n", sum_abs);
        Serial.flush();
    }

    for (int i = 0; i < AP_FRAME_SAMPLES; ++i) {
        int32_t s = raw32[i] >> 8;  // 24-bit to 16-bit
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        pcm_out[i] = (int16_t)s;
        if (show_debug && i < 4) {
            Serial.printf("DEBUG: Sample %d: raw=0x%08x shifted=%d\n", i, raw32[i], pcm_out[i]);
        }
    }

    if (show_debug) {
        Serial.println("DEBUG: readFrame succeeded");
        Serial.flush();
    }
    debug_count++;
    return true;
}
