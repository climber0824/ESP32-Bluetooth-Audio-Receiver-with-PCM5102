#include "pcm5102_driver.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PCM5102";
static i2s_port_t g_i2s_num = I2S_NUM_0;

pcm5102_config_t pcm5102_get_default_config(void) {
    pcm5102_config_t config = {
        .bck_io_num = PCM5102_DEFAULT_BCK_IO,
        .ws_io_num = PCM5102_DEFAULT_WS_IO,
        .data_out_num = PCM5102_DEFAULT_DATA_OUT_IO,
        .sample_rate = PCM5102_DEFAULT_SAMPLE_RATE,
        .bits_per_sample = PCM5102_DEFAULT_BITS_PER_SAMPLE,
        .channel_format = PCM5102_DEFAULT_CHANNEL_FORMAT,
        .i2s_num = PCM5102_I2S_NUM,
        .dma_buf_count = PCM5102_DMA_BUF_COUNT,
        .dma_buf_len = PCM5102_DMA_BUF_LEN
    };
    return config;
}

esp_err_t pcm5102_init(const pcm5102_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    g_i2s_num = config->i2s_num;

    // I2S config
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = config->sample_rate,
        .bits_per_sample = config->bits_per_sample,
        .channel_format = config->channel_format,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = config->dma_buf_count,
        .dma_buf_len = config->dma_buf_len,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // Install I2S driver
    esp_err_t ret = i2s_driver_install(config->i2s_num, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return ret;
    }

    // Pin config
    i2s_pin_config_t pin_config = {
        .bck_io_num = config->bck_io_num,
        .ws_io_num = config->ws_io_num,
        .data_out_num = config->data_out_num,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    ret = i2s_set_pin(config->i2s_num, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(config->i2s_num);
        return ret;
    }

    ESP_LOGI(TAG, "PCM5102 initialized successfully");
    ESP_LOGI(TAG, "Sample rate: %ld Hz", config->sample_rate);
    ESP_LOGI(TAG, "Bits per sample: %d", config->bits_per_sample);
    ESP_LOGI(TAG, "BCK: GPIO%d, WS: GPIO%d, DATA: GPIO%d", 
             config->bck_io_num, config->ws_io_num, config->data_out_num);

    return ESP_OK;
}

esp_err_t pcm5102_deinit(void) {
    esp_err_t ret = i2s_driver_uninstall(g_i2s_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall I2S driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "PCM5102 deinitialized");
    return ESP_OK;
}

esp_err_t pcm5102_write(const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait) {
    if (src == NULL) {
        ESP_LOGE(TAG, "Source buffer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2s_write(g_i2s_num, src, size, bytes_written, ticks_to_wait);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t pcm5102_set_sample_rate(uint32_t sample_rate) {
    esp_err_t ret = i2s_set_sample_rates(g_i2s_num, sample_rate);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Sample rate changed to %ld Hz", sample_rate);
    return ESP_OK;
}

esp_err_t pcm5102_start(void) {
    esp_err_t ret = i2s_start(g_i2s_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2S started");
    return ESP_OK;
}

esp_err_t pcm5102_stop(void) {
    esp_err_t ret = i2s_stop(g_i2s_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop I2S: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2S stopped");
    return ESP_OK;
}

esp_err_t pcm5102_clear_buffer(void) {
    esp_err_t ret = i2s_zero_dma_buffer(g_i2s_num);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear DMA buffer: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}
