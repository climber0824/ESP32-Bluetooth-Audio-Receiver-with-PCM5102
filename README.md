# ESP32-Bluetooth-Audio-Receiver-with-PCM5102
Complete Bluetooth A2DP audio receiver system for ESP32 that receives audio wirelessly from any Bluetooth device and plays it through the PCM5102 DAC.

## Features

✅ Bluetooth A2DP Audio Sink (receive audio wirelessly)  
✅ Supports MP3, MP4, AAC, and any format the source device can decode  
✅ High-quality I2S audio output through PCM5102 DAC  
✅ Automatic reconnection support  
✅ Volume control  
✅ Metadata display (song title, artist, album)  
✅ AVRCP support (play/pause/next/previous)  
✅ FreeRTOS task-based architecture  
✅ Connection status callbacks  

## Hardware Requirements

### ESP32 Module
- NodeMCU ESP32 or similar
- Minimum 4MB flash recommended
- Bluetooth Classic support (all ESP32 modules have this)

### PCM5102 DAC Module
- PCM5102 I2S DAC breakout board
- Power: 3.3V from ESP32

### Audio Output
- Passive speakers or amplified speakers
- 3.5mm audio jack (if using breakout with jack)
- Or direct RCA/wire connection

## Wiring Diagram

### ESP32 to PCM5102 Connections

| PCM5102 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VIN         | 3.3V      | Power supply |
| GND         | GND       | Ground |
| LCK (LRCK)  | GPIO 25   | Word Select (Left/Right Clock) |
| BCK         | GPIO 26   | Bit Clock |
| DIN         | GPIO 22   | Data Input |
| SCK         | GND       | System Clock (internal mode) |
| FLT         | GND       | Filter Select (normal latency) |
| FMT         | GND       | Audio Format (I2S) |
| XMT         | 3.3V      | Soft Mute (unmute) |

### PCM5102 to Speakers

| PCM5102 Pin | Connection |
|-------------|------------|
| LOUT        | Left channel positive |
| LGND        | Left channel ground |
| ROUT        | Right channel positive |
| RGND        | Right channel ground |

**Note:** PCM5102 has line-level output. For best results:
- Use powered/active speakers
- Or connect to an amplifier
- Some modules have 3.5mm jack built-in

## Software Setup

### Prerequisites

- ESP-IDF v4.4 or later
- Python 3.6+
- ESP-IDF tools installed and configured

### Project Structure

```
project/
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── bt_audio_receiver.c
│   ├── bt_audio_receiver.h
│   ├── pcm5102_driver.c
│   └── pcm5102_driver.h
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

### Building the Firmware

1. **Clone or copy the project files**

2. **Set ESP-IDF environment**
```bash
. $HOME/esp/esp-idf/export.sh
```

3. **Configure the project** (optional)
```bash
idf.py menuconfig
```

4. **Build the project**
```bash
idf.py build
```

5. **Flash to ESP32**
```bash
idf.py -p /dev/ttyUSB0 flash
```

6. **Monitor output**
```bash
idf.py -p /dev/ttyUSB0 monitor
```

### Configuration Options

#### Change Bluetooth Device Name

In `main.c`:
```c
strncpy(bt_config.device_name, "ESP32_Speaker", sizeof(bt_config.device_name) - 1);
```

Change `"ESP32_Speaker"` to your desired name.

#### Change I2S Pins

In `pcm5102_driver.h`:
```c
#define PCM5102_DEFAULT_BCK_IO      26  // Change as needed
#define PCM5102_DEFAULT_WS_IO       25  // Change as needed
#define PCM5102_DEFAULT_DATA_OUT_IO 22  // Change as needed
```

#### Adjust Audio Buffer

In `bt_audio_receiver.h`:
```c
#define BT_AUDIO_BUFFER_SIZE 4096  // Increase for stability, decrease for lower latency
```

## Usage

### 1. Power On ESP32

After flashing, the ESP32 will:
- Initialize Bluetooth
- Initialize PCM5102
- Become discoverable as "ESP32_Speaker" (or your custom name)
- Wait for connections

### 2. Connect from Raspberry Pi

**Quick connect:**
```bash
bluetoothctl
scan on
# Wait for ESP32_Speaker to appear
pair <MAC_ADDRESS>
connect <MAC_ADDRESS>
```

### 3. Play Audio

Once connected, any audio played on the Raspberry Pi will stream to the ESP32.

```bash
# Play MP3
mpg123 song.mp3

# Play MP4 audio
ffplay -nodisp song.mp4

# Play with VLC
cvlc song.mp3
```

### 4. Monitor ESP32

Through serial monitor, you'll see:
```
Status: 🔍 Waiting for connection...
📱 Device connected! Now playing...
🎵 Now Playing:
   Title:  Song Name
   Artist: Artist Name
   Album:  Album Name
Status: ▶️  Playing audio (Volume: 50%)
```

## API Reference

### Bluetooth Audio Functions

```c
// Initialize Bluetooth audio receiver
bt_audio_config_t config = bt_audio_get_default_config();
esp_err_t bt_audio_init(const bt_audio_config_t *config);

// Start (make discoverable)
esp_err_t bt_audio_start(void);

// Stop and disconnect
esp_err_t bt_audio_stop(void);

// Clean up
esp_err_t bt_audio_deinit(void);

// Get current state
bt_audio_state_t bt_audio_get_state(void);

// Volume control (0-100)
esp_err_t bt_audio_set_volume(uint8_t volume);
uint8_t bt_audio_get_volume(void);

// AVRCP commands
esp_err_t bt_audio_avrcp_cmd(esp_avrc_pt_cmd_t cmd);
```

### Callbacks

```c
// Connection state callback
void on_connection_state_changed(bool connected) {
    if (connected) {
        // Device connected
    } else {
        // Device disconnected
    }
}

// Metadata callback
void on_metadata_received(const char *title, const char *artist, const char *album) {
    // New song metadata received
}
```

### PCM5102 Functions

```c
// Initialize PCM5102
pcm5102_config_t pcm_config = pcm5102_get_default_config();
esp_err_t pcm5102_init(const pcm5102_config_t *config);

// Write audio data
esp_err_t pcm5102_write(const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait);

// Control functions
esp_err_t pcm5102_set_sample_rate(uint32_t sample_rate);
esp_err_t pcm5102_start(void);
esp_err_t pcm5102_stop(void);
esp_err_t pcm5102_clear_buffer(void);
```

## Advanced Features

### Custom Volume Curve

Modify the volume control in `bt_audio_receiver.c`:

```c
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    // Custom volume curve
    float volume_factor = pow(g_volume / 100.0f, 2.0f);  // Exponential curve
    
    int16_t *samples = (int16_t *)data;
    size_t sample_count = len / 2;
    
    for (size_t i = 0; i < sample_count; i++) {
        samples[i] = (int16_t)(samples[i] * volume_factor);
    }
    
    // Write to DAC
    size_t bytes_written;
    pcm5102_write(data, len, &bytes_written, 0);
}
```

### Audio Effects

Add simple effects in the data callback:

```c
// Mono conversion (mix left and right)
int16_t *samples = (int16_t *)data;
for (size_t i = 0; i < len/4; i++) {
    int16_t mixed = (samples[i*2] + samples[i*2+1]) / 2;
    samples[i*2] = samples[i*2+1] = mixed;
}

// Simple low-pass filter
static int16_t prev_left = 0, prev_right = 0;
for (size_t i = 0; i < len/4; i++) {
    samples[i*2] = (samples[i*2] + prev_left) / 2;
    samples[i*2+1] = (samples[i*2+1] + prev_right) / 2;
    prev_left = samples[i*2];
    prev_right = samples[i*2+1];
}
```

### Button Control

Add physical buttons for volume and playback:

```c
#include "driver/gpio.h"

#define BUTTON_PLAY_PAUSE  GPIO_NUM_0
#define BUTTON_VOL_UP      GPIO_NUM_4
#define BUTTON_VOL_DOWN    GPIO_NUM_5

void button_task(void *pvParameters) {
    // Configure GPIO
    gpio_set_direction(BUTTON_PLAY_PAUSE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PLAY_PAUSE, GPIO_PULLUP_ONLY);
    
    while (1) {
        if (gpio_get_level(BUTTON_PLAY_PAUSE) == 0) {
            bt_audio_avrcp_cmd(ESP_AVRC_PT_CMD_PLAY);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

## Troubleshooting

### ESP32 Not Discoverable

- Check Bluetooth is enabled in menuconfig
- Verify `ESP_BT_MODE_CLASSIC_BT` is set
- Check serial output for initialization errors
- Power cycle the ESP32

### No Audio Output

- Verify PCM5102 wiring (especially BCK, WS, DIN)
- Check PCM5102 configuration pins (FMT=GND, SCK=GND, XMT=3.3V)
- Verify speakers are connected and powered
- Check serial output for I2S errors
- Test with simple tone generator first

### Audio Stuttering/Skipping

- Increase `BT_AUDIO_BUFFER_SIZE` in bt_audio_receiver.h
- Increase DMA buffer count in pcm5102_driver.h
- Check WiFi is not enabled (can cause interference)
- Reduce distance between Raspberry Pi and ESP32
- Check power supply stability (use external 5V supply, not USB)

### Connection Drops

- Ensure ESP32 is not resetting (check power supply)
- Disable power-saving modes
- Check for WiFi/Bluetooth interference
- Keep devices within reasonable range (<10m)

### Audio Quality Issues

- Verify sample rate matches source (usually 44.1kHz)
- Check for EMI/noise on power lines
- Use shielded cable for long I2S connections
- Add capacitors on PCM5102 power line (100nF + 10µF)

### High Latency

- Reduce `BT_AUDIO_BUFFER_SIZE` (trade-off with stability)
- Reduce `dma_buf_len` in PCM5102 config
- Note: Bluetooth A2DP has inherent ~100-200ms latency

## Performance Optimization

### Memory Usage

Current memory footprint:
- Bluetooth stack: ~50KB RAM
- Audio buffers: ~8KB RAM
- FreeRTOS tasks: ~8KB RAM
- Total: ~66KB RAM (ESP32 has 520KB)

### CPU Usage

- Bluetooth processing: ~20-30%
- Audio decoding: Done on source device
- I2S DMA: Hardware-accelerated
- Volume processing: <5%

### Power Consumption

- Active (playing): ~150-200mA @ 3.3V
- Connected (idle): ~100-120mA @ 3.3V
- Discoverable: ~80-100mA @ 3.3V

## Compatible Devices

### Audio Formats

The ESP32 receives decoded PCM audio via Bluetooth A2DP. The source device handles decoding, so all formats supported by the source work:

- MP3
- MP4/M4A (AAC)
- WAV
- FLAC
- OGG Vorbis
- WMA
- And more...

## Future Enhancements

Possible additions:
- [ ] Web-based configuration interface (WiFi)
- [ ] SD card audio playback
- [ ] Multiple device pairing
- [ ] Audio visualization (VU meter)
- [ ] Equalizer settings
- [ ] Bluetooth audio source mode (transmit)
- [ ] Battery level monitoring
- [ ] OLED display for status/metadata

## License

This project is provided as-is for educational and personal use.

## References

- [ESP32 Bluetooth A2DP Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_a2dp.html)
- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [PCM5102 Datasheet](https://www.ti.com/lit/ds/symlink/pcm5102.pdf)
- [A2DP Specification](https://www.bluetooth.com/specifications/specs/advanced-audio-distribution-profile/)

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review ESP32 serial output for error messages
3. Verify wiring and component functionality
4. Test individual components (Bluetooth, I2S) separately

## Acknowledgments

Built with ESP-IDF and inspired by the ESP32 audio projects community.
