#include <driver/i2s.h>
#include <M5Atom.h>

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAKER_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1
#define DATA_SIZE 1024

uint8_t microphonedata0[1024 * 80];
int data_offset = 0;

// Used to stop endless loops, so that I have time to read the debug :(
bool disabled_for_debug = true;

bool isRecording = false;
unsigned long recordStartTime = 0;
unsigned long maxRecordingDuration = 2000;
int soundThreshold = 100;
unsigned long maxSilenceDuration = 1000;

bool isPlaying = false;
unsigned long playbackStartTime = 0;

void InitI2SSpeakerOrMic(int mode)
{
    esp_err_t err = ESP_OK;

    //Serial.print("Uninstalling the I2S driver, on port: ");
    //Serial.println(SPEAKER_I2S_NUMBER);
    i2s_driver_uninstall(SPEAKER_I2S_NUMBER);

    // Initialize the I2S configuration structure
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
    };

    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    // Install the I2S driver with the specified configuration
    err += i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL);

    // Configure pin assignments for I2S
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    tx_pin_config.mck_io_num = GPIO_NUM_0; // Master clock (MCK) GPIO, set to GPIO_NUM_0

    //Serial.print("Setting pin configuration for I2S, port: ");
    //Serial.println(SPEAKER_I2S_NUMBER);
    err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);

    //Serial.print("Setting clock configuration for I2S, port: ");
    //Serial.println(SPEAKER_I2S_NUMBER);
    err += i2s_set_clk(SPEAKER_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void startRecording()
{
    Serial.println("Recording started");
    isRecording = true;
    data_offset = 0;
    recordStartTime = millis();
    InitI2SSpeakerOrMic(MODE_MIC);
}

void stopRecordingAndStartPlayback()
{
    Serial.println("Recording stopped");
    isRecording = false;

    InitI2SSpeakerOrMic(MODE_SPK);

    Serial.println("Playback started");
    isPlaying = true;

    playbackStartTime = millis();
    size_t bytes_written;
    i2s_write(SPEAKER_I2S_NUMBER, microphonedata0, data_offset, &bytes_written, portMAX_DELAY);
}

void stopPlayback()
{
    Serial.print("Playback stopped, ms: ");
    Serial.println(millis() - playbackStartTime);
    isPlaying = false;
}

void setup() {
    M5.begin(true, false, true);
    Serial.begin(115200);
    M5.dis.drawpix(0, CRGB(128, 128, 0));
    delay(2000);
}

void loop() {
    M5.update();

    if (!disabled_for_debug)
    {
      // Check for sound level and start recording if sound is detected
      int soundLevel = analogRead(G0);

      Serial.print("Sound Level: ");
      Serial.println(soundLevel);

      if (soundLevel > soundThreshold && !isRecording)
      {
          startRecording();
      }

      if (isRecording)
      {
          unsigned long elapsedRecordTime = millis() - recordStartTime;
          /*
          Serial.print("is recording, (");
          Serial.print(elapsedRecordTime);
          Serial.println(")");
          */
          size_t byte_read;
          i2s_read(SPEAKER_I2S_NUMBER, (char *)(microphonedata0 + data_offset), DATA_SIZE, &byte_read, (100 / portTICK_RATE_MS));
          data_offset += byte_read;

          // Check for silence or end of recording duration
          if ((elapsedRecordTime > maxRecordingDuration) || (byte_read == 0 && millis() - recordStartTime > maxSilenceDuration))
          {
              stopRecordingAndStartPlayback();
          }
      }

      // Stop playback after a certain duration
      if (isPlaying == true && millis() - recordStartTime > maxSilenceDuration)
      {
          stopPlayback();
      }

    }
}
