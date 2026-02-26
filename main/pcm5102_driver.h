#ifndef PCM5102_DRIVER_H
#define PCM5102_DRIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default I2C pin configuration for PCM5102
#define PCM5102_DEFAULT_BCK_IO      26  // Bit clock
#define PCM5102_DEFAULT_WS_IO       25  // Word select (LRCK)
#define PCM5102_DEFAULT_DATA_OUT_IO 22  // Data out

// Default audio configuration
#define PCM5102_DEFAULT_SAMPLE_RATE 44100
#define PCM5102_DEFAULT_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define PCM5102_DEFAULT_CHANNEL_FORMAT  I2S_CHANNEL_FMT_RIGHT_LEFT

// I2S port number
#define PCM5102_I2S_NUM I2S_NUM_0

// DMA buffer config
#define PCM5102_DMA_BUF_COUNT 8
#define PCM5102_DMA_BUF_LEN   1024

/**
 * @brief PCM5102 config structure
 */
typedef struct {
    int bck_io_num;              // BCK (Bit Clock) pin
    int ws_io_num;               // WS (Word Select/LRCK) pin
    int data_out_num;            // DATA pin
    uint32_t sample_rate;        // Sample rate (e.g., 44100, 48000)
    i2s_bits_per_sample_t bits_per_sample;  // Bits per sample
    i2s_channel_fmt_t channel_format;       // Channel format
    i2s_port_t i2s_num;          // I2S port number
    int dma_buf_count;           // DMA buffer count
    int dma_buf_len;             // DMA buffer length
} pcm5102_config_t;

/**
 * @brief Get default PCM5102 config
 * 
 * @return pcm5102_config_t Default config

*/
pcm5102_config_t pcm5102_get_default_config(void);


/**
 * @brief Initialize PCM5102 driver
 * @param config pointer to config structure
 * 
 * @return esp_err_t ESP_OK on success
*/
esp_err_t pcm5102_init(const pcm5102_config_t *config);

/**
 * @brief Deinitialize PCM5102 driver
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_deinit(void);

/**
 * @brief Write audio data to PCM5102
 * 
 * @param src Pointer to source data buffer
 * @param size Size of data to write in bytes
 * @param bytes_written Pointer to store actual bytes written
 * @param ticks_to_wait Ticks to wait for write operation
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_write(const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait);

/**
 * @brief Set sample rate
 * 
 * @param sample_rate New sample rate
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_set_sample_rate(uint32_t sample_rate);

/**
 * @brief Start I2S transmission
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_start(void);

/**
 * @brief Stop I2S transmission
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_stop(void);

/**
 * @brief Clear I2S DMA buffer
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pcm5102_clear_buffer(void);

#ifdef __cplusplus
}
#endif

#endif // PCM5102_DRIVER_H
