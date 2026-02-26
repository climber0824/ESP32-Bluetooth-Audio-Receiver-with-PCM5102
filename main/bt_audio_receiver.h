#ifndef BT_AUDIO_RECEIVER_H
#define BT_AUDIO_RECEIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bluetooth device name (shown on Raspberry Pi)
#define BT_DEVICE_NAME "ESP32_Audio_Player"

// Audio buffer configuration
#define BT_AUDIO_BUFFER_SIZE 4096
#define BT_AUDIO_QUEUE_SIZE 10

/**
 * @brief Bluetooth audio receiver configuration
 */
typedef struct {
    char device_name[32];           // Bluetooth device name
    bool auto_reconnect;            // Auto reconnect on disconnect
    void (*connection_callback)(bool connected);  // Connection state callback
    void (*metadata_callback)(const char *title, const char *artist, const char *album);  // Metadata callback
} bt_audio_config_t;

/**
 * @brief Connection state
 */
typedef enum {
    BT_AUDIO_DISCONNECTED = 0,
    BT_AUDIO_CONNECTING,
    BT_AUDIO_CONNECTED,
    BT_AUDIO_PLAYING
} bt_audio_state_t;

/**
 * @brief Get default Bluetooth audio configuration
 * 
 * @return bt_audio_config_t Default configuration
 */
bt_audio_config_t bt_audio_get_default_config(void);

/**
 * @brief Initialize Bluetooth audio receiver
 * 
 * @param config Pointer to configuration structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_init(const bt_audio_config_t *config);

/**
 * @brief Start Bluetooth audio receiver (make discoverable)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_start(void);

/**
 * @brief Stop Bluetooth audio receiver
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_stop(void);

/**
 * @brief Deinitialize Bluetooth audio receiver
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_deinit(void);

/**
 * @brief Get current connection state
 * 
 * @return bt_audio_state_t Current state
 */
bt_audio_state_t bt_audio_get_state(void);

/**
 * @brief Disconnect current Bluetooth connection
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_disconnect(void);

/**
 * @brief Set volume (0-100)
 * 
 * @param volume Volume level
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_set_volume(uint8_t volume);

/**
 * @brief Get current volume
 * 
 * @return uint8_t Current volume (0-100)
 */
uint8_t bt_audio_get_volume(void);

/**
 * @brief Send AVRCP(Audio/Video Remote Control Profile) command (play/pause/next/prev)
 * 
 * @param cmd AVRCP command
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bt_audio_avrcp_cmd(esp_avrc_pt_cmd_t cmd);

#ifdef __cplusplus
}
#endif

#endif // BT_AUDIO_RECEIVER_H
