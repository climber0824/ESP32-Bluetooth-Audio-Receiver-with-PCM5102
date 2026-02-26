#include "bt_audio_receiver.h"
#include "pcm5102_driver.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "BT_AUDIO";

// Global state
static bt_audio_state_t g_state = BT_AUDIO_DISCONNECTED;
static uint8_t g_volume = 50;
static bt_audio_config_t g_config;
static bool g_initialized = false;

// Forward declarations
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

bt_audio_config_t bt_audio_get_default_config(void) {
    bt_audio_config_t config = {0};
    strncpy(config.device_name, BT_DEVICE_NAME, sizeof(config.device_name) - 1);
    config.auto_reconnect = true;
    config.connection_callback = NULL;
    config.metadata_callback = NULL;

    return config;
}

esp_err_t bt_audio_init(const bt_audio_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (g_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Store config
    memcpy(&g_config, config, sizeof(bt_audio_config_t));

    // Initialize NVS for Bluetooth
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set device name
    esp_bt_dev_set_device_name(g_config.device_name);

    // Initialize A2DP sink
    ret = esp_a2d_register_callback(bt_app_a2d_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "A2DP sink init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize AVRCP controller
    ret = esp_avrc_ct_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AVRCP controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AVRCP register callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register GAP callback
    ret = esp_bt_gap_register_callback(bt_app_gap_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP register callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set discoverable and connectable mode
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // Initialize PCM5102
    pcm5102_config_t pcm_config = pcm5102_get_default_config();
    pcm_config.sample_rate = 44100;
    ret = pcm5102_init(&pcm_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCM5102 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    g_initialized = true;

    ESP_LOGI(TAG, "Bluetooth audio receiver initialized as '%s'", g_config.device_name);

    return ESP_OK;
}

esp_err_t bt_audio_start(void) {
    if (!g_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Bluetooth audio receiver started - discoverable as '%s'", g_config.device_name);
    return ESP_OK;
} 

esp_err_t bt_audio_stop(void) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;        
    }

    esp_a2d_sink_disconnect(NULL);
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

    ESP_LOGI(TAG, "Bluetooth audio receiver stopped");
    return ESP_OK;
}

esp_err_t bt_audio_deinit(void) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;        
    }

    bt_audio_stop();
    esp_a2d_sink_deinit();
    esp_avrc_ct_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    pcm5102_deinit();

    g_initialized = false;
    g_state = BT_AUDIO_DISCONNECTED;

    ESP_LOGI(TAG, "Bluetooth audio receiver deinitialized");
    return ESP_OK;
}

bt_audio_state_t bt_audio_get_state(void) {
    return g_state;
}

esp_err_t bt_audio_disconnect(void) {
    return esp_a2d_sink_disconnect(NULL);
}

esp_err_t bt_audio_set_volume(uint8_t volume) {    
    g_volume = volume > 100 ? 100 : volume;
    ESP_LOGI(TAG, "Volume set to %d%%", volume);
    return ESP_OK;
}

uint8_t bt_audio_get_volume(void) {
    return g_volume;
}

esp_err_t bt_audio_avrcp_cmd(esp_avrc_pt_cmd_t cmd) {
    return esp_avrc_ct_send_passthrough_cmd(0, cmd, ESP_AVRC_PT_CMD_STATE_PRESSED);
}

// A2DP callback
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "A2DP connection state: %d", param->conn_stat.state);
        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            g_state = BT_AUDIO_CONNECTED;
            ESP_LOGI(TAG, "A2DP connected");
            if (g_config.connection_callback) {
                g_config.connection_callback(true);
            }
        }
        else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            g_state = BT_AUDIO_DISCONNECTED;
            ESP_LOGI(TAG, "A2DP disconnected");
            if (g_config.connection_callback) {
                g_config.connection_callback(false);
            }
            pcm5102_clear_buffer();
        }
        break;
    
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(TAG, "A2DP audio state: %d", param->audio_stat.state);
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            g_state = BT_AUDIO_PLAYING;
            ESP_LOGI(TAG, "Audio playback started");
        }
        else if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STOPPED) {
            g_state = BT_AUDIO_CONNECTED;
            ESP_LOGI(TAG, "Audio playback stopped");
            pcm5102_clear_buffer();
        }
        break;

    case ESP_A2D_AUDIO_CFG_EVT:
        ESP_LOGI(TAG, "A2DP audio codec configured");
        ESP_LOGI(TAG, "Sample rate: %d", param->audio_cfg.mcc.cie.sbc[0]);
        break;

    default:
        ESP_LOGD(TAG, "A2DP event: %d", event);
        break;
    }
}

// A2DP data callback - receiving audio data
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len) {
    if (data == NULL || len == 0) {
        return;
    }

    // apply volume control
    if (g_volume <= 100) {
        int16_t *samples = (int16_t *)data;
        size_t sample_count = len / 2;
        float volume_factor = g_volume / 100.0f;

        for (size_t i = 0; i < sample_count; i++) {
            samples[i] = (int16_t)(samples[i] * volume_factor);
        }
    }

    // write to pcm5102
    size_t bytes_written;
    esp_err_t ret = pcm5102_write(data, len, &bytes_written, 0);

    if (ret != ESP_OK || bytes_written < len) {
        // Buffer full or error - this is normal, just skip
        ESP_LOGD(TAG, "I2S buffer full, %d bytes written of %d", bytes_written, len);
    }
}

// AVRCO callback
static void bt_app_re_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "AVRCP connection state: %d", param->conn_stat.connected);
        break;

    case ESP_AVRC_CT_METADATA_RSP_EVT:
        if (g_config.metadata_callback && param->meta_rsp.attr_id <= ESP_AVRC_MD_ATTR_ALBUM) {
            char *attr_text = (char *)param->meta_rsp.attr_text;
            ESP_LOGI(TAG, "Metadata [%d]: %s", param->meta_rsp.attr_id, attr_text);
            
            // Store metadata (simplified)
            static char title[128] = {0};
            static char artist[128] = {0};
            static char album[128] = {0};

            switch (param->meta_rsp.attr_id)
            {
            case ESP_AVRC_MD_ATTR_TITLE:
                strncpy(title, attr_text, sizeof(title) - 1);
                break;
            
            case ESP_AVRC_MD_ATTR_ARTIST:
                strncpy(artist, attr_text, sizeof(artist) - 1);

            case ESP_AVRC_MD_ATTR_ALBUM:
                strncpy(album, attr_text, sizeof(album) - 1);                
                break;
            }
            
            g_config.metadata_callback(title, artist, album);
        }
        break;

    case ESP_AVRC_CT_PLAY_STATUS_RSP_EVT:
        ESP_LOGD(TAG, "AVRCP passthrough response: %d", param->psth_rsp.key_code);
        break;

    default:
        ESP_LOGD(TAG, "AVRCP event: %d", event);
        break;
    }
}

// GAP callback
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event)
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Authentication success: %s", param->auth_cmpl.device_name);
        }
        else {
            ESP_LOGE(TAG, "Authentication failed, status: %d", param->auth_cmpl.stat);
        }
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
        ESP_LOGI(TAG, "PIN code request");
        // Auto accept with default PIN 0000
        esp_bt_pin_code_t pin_code = {'0', '0', '0', '0'};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        break;

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "GAP mode changed: %d", param->mode_chg.mode);
        break;
    
    default:
        ESP_LOGD(TAG, "GAP event: %d", event);
        break;
    }
}
