#include <Arduino.h>
#include "AudioCapture.h"
#include "env.h"
#include <driver/i2s.h>

bool AudioCapture::begin()
{
    configureI2S();
    return true; // Simplified; add error checking if needed
}

int16_t AudioCapture::readSample()
{
    uint32_t raw;
    size_t bytesRead = 0;

    // Read 32-bit sample from I2S
    esp_err_t result = i2s_read(I2S_NUM_0, &raw, sizeof(raw), &bytesRead, portMAX_DELAY);
    if (result == ESP_OK && bytesRead == sizeof(raw))
    {
        // Shift down 24-bit mic data to 16-bit range
        int16_t sample = (int16_t)(raw >> 14);
        return sample;
    }
    return 0; // Error or no data
}

void AudioCapture::configureI2S()
{
    // I2S configuration
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,   // INMP441 is 24-bit, align to 32
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,    // use left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };


    // I2S pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN, // GPIO4
        .ws_io_num = I2S_LRCL_PIN,  // GPIO5
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DOUT_PIN // GPIO6
    };

    // Install and start I2S driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}
