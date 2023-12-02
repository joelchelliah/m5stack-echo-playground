#include <driver/i2s.h>
#include <M5Atom.h>

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

// Sound recording, analysis and calibration
#define DATA_SIZE 1024
#define GAIN_FACTOR 1
#define DYNAMIC_SOUND_THRESHOLD_MULTIPLIER 2
#define MIN_DYNAMIC_SOUND_THRESHOLD 2500
#define MAX_RECORDING_DURATION 2000
#define MAX_SILENCE_DURATION 1000

bool isPaused = true;

uint8_t microphonedata0[1024 * 80];
int data_offset = 0;

// Recording control variables
bool isListening = false;
bool isRecording = false;
unsigned long recordStartTime = 0;
unsigned long elapsedRecordTime = 0;

// Silence control variables
unsigned long silenceStartTime = 0;
unsigned long elapsedSilenceTime = 0;

// Sound level calibration variables
bool isSoundLevelCalibrated = false;
int averageSoundLevelData = 0;
int averageSoundLevelCount = 0;
int dynamicSoundThreshold = 0;

// Playback control variables
bool isPlaying = false;
unsigned long playbackStartTime = 0;
unsigned long elapsedPlaybackTime = 0;


void InitI2SSpeakerOrMic(int mode) {
    esp_err_t err = ESP_OK;

    //Serial.print("Uninstalling the I2S driver, on port: ");
    //Serial.println(I2S_NUMBER);
    i2s_driver_uninstall(I2S_NUMBER);

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
    err += i2s_driver_install(I2S_NUMBER, &i2s_config, 0, NULL);

    // Configure pin assignments for I2S
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    tx_pin_config.mck_io_num = GPIO_NUM_0; // Master clock (MCK) GPIO, set to GPIO_NUM_0

    err += i2s_set_pin(I2S_NUMBER, &tx_pin_config);
    err += i2s_set_clk(I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void startListening() {
    Serial.println("- Listening started -");
    isListening = true;
    isRecording = false;
    isPlaying = false;
    data_offset = 0;

    InitI2SSpeakerOrMic(MODE_MIC);
}

void startRecording() {
    if(!isListening) {
      throw std::runtime_error("Tried to record when not listening!");
    }

    Serial.println("- Recording started -");
    isRecording = true;
    elapsedRecordTime = 0;
    recordStartTime = millis();
}

void stopListeningAndRecordingAndStartPlayback() {
    Serial.print("Recording stopped, ms:");
    Serial.println(elapsedRecordTime);
    isListening = false;
    isRecording = false;

    InitI2SSpeakerOrMic(MODE_SPK);

    Serial.println("- Playback started -");
    isPlaying = true;

    elapsedPlaybackTime = 0;
    playbackStartTime = millis();

    size_t bytes_written;
    i2s_write(I2S_NUMBER, microphonedata0, data_offset, &bytes_written, portMAX_DELAY);
}

void stopPlayback() {
    Serial.print("Playback stopped, ms: ");
    Serial.println(elapsedPlaybackTime);
    isPlaying = false;
}

void collectSoundLevelCalibrationData(int16_t *adcBuffer) {
  int soundLevel;
  int startPoint = 10; // Skip the first x samples because of mysterious noise.
  
  // Reset sound threshold calibration data
  isSoundLevelCalibrated = false;
  averageSoundLevelData = 0;
  averageSoundLevelCount = 0;

  for (int n = startPoint; n < DATA_SIZE / sizeof(int16_t); n++) {
    soundLevel = adcBuffer[n] * GAIN_FACTOR;
    averageSoundLevelData += soundLevel;
    averageSoundLevelCount++;
  }
}

void setSoundLevelBasedOnCalibrationData() {
  if(isSoundLevelCalibrated) {
    return;
  }

  isSoundLevelCalibrated = true;
  int averageSoundLevel = averageSoundLevelData / averageSoundLevelCount;
  dynamicSoundThreshold = DYNAMIC_SOUND_THRESHOLD_MULTIPLIER * averageSoundLevel;

  if(dynamicSoundThreshold < MIN_DYNAMIC_SOUND_THRESHOLD) {
    dynamicSoundThreshold = MIN_DYNAMIC_SOUND_THRESHOLD;
  }

  Serial.print("Average Sound Level: ");
  Serial.println(averageSoundLevel);
  Serial.print("Dynamic Threshold: ");
  Serial.println(dynamicSoundThreshold);
}

bool wasAboveSoundThreshold(int16_t *adcBuffer) {
  int soundLevel;
  int startPoint = 5; //Skip the first x samples because of mysterious noise.
  int numPoints = DATA_SIZE / sizeof(int16_t);
  int soundsAboveThreshold[numPoints];
  int count = 0;


  for (int n = startPoint; n < numPoints; n++) {
    soundLevel = adcBuffer[n] * GAIN_FACTOR;

    if (soundLevel > dynamicSoundThreshold) {
      Serial.print("Sound level: ");
      Serial.println(soundLevel);
      soundsAboveThreshold[count++] = soundLevel;
    }
  }

  if (count > 5) {
    Serial.println("- Above sound threshold -");
  }

  return count > 5;
}

void showPausedMessage() {
  Serial.println(" - - - - Paused - - - - -");
  Serial.println(" - - ( calibrating ) - - ");
  Serial.println("- Press button to start -");
}

void setup() {
    M5.begin(true, false, true);
    Serial.begin(115200);
    M5.dis.drawpix(0, CRGB(128, 128, 0));
    delay(2000);
    startListening();

    if(isPaused) {
      showPausedMessage();
    }
}

void loop() {
    M5.update();

    // Listening-mode
    if (isListening) {
        // Where to store microphone input from this iteration.
        // The offset points to exactly where in the array the storing should start for this iteration.
        uint8_t *buffer = microphonedata0 + data_offset;
        size_t bytes_read;

        // Store data from microphone input. Will only be kept if the offset is moved before the next iteration.
        // Only keep if:
        //  - Recording-mode has already been triggered.
        //  - sound passes volume threshold, and recording-mode will be triggered in this iteration.
        unsigned long recordStartTimeLocal = millis();
        i2s_read(I2S_NUMBER, (char *)buffer, DATA_SIZE, &bytes_read, (100 / portTICK_RATE_MS));
        unsigned long elapsedRecordTimeLocal = millis() - recordStartTimeLocal;

        // Interpret buffer as 16-bit signed integers for easier processing
        int16_t *adcBuffer = (int16_t*)buffer;

        if (M5.Btn.wasPressed()) {
          isPaused = !isPaused;

          if(isPaused) {
            showPausedMessage();
          } else {
            data_offset = 0;
          }
        }

        if(isPaused) {
          collectSoundLevelCalibrationData(adcBuffer);
          return;
        } else {
          setSoundLevelBasedOnCalibrationData();
        }

        bool wasSoundDetected = wasAboveSoundThreshold(adcBuffer);

        // Decide if recording-mode should be triggered
        if(!isRecording) {
          if (wasSoundDetected) {
            startRecording();
          } else {
            Serial.println("Still listening...");
          }
        }

        // Move the offset, so that the stored microphone data is kept, and doesn't get overwritten
        if(isRecording)
        {
          data_offset += bytes_read;
          elapsedRecordTime += elapsedRecordTimeLocal;

          // Check for silence, or update silence duration, if no sound was detected
          if(wasSoundDetected) {
            silenceStartTime = 0;
            elapsedSilenceTime = 0;
          } else {
            Serial.print("Silence, ms: ");
            Serial.println(elapsedSilenceTime);

            if(silenceStartTime == 0) {
              silenceStartTime = millis();
            } else {
              elapsedSilenceTime = millis() - silenceStartTime;
            }
          }

          // Check end of recording duration or end of silence duration
          if ((elapsedRecordTime > MAX_RECORDING_DURATION) || elapsedSilenceTime > MAX_SILENCE_DURATION)
          {
              stopListeningAndRecordingAndStartPlayback();
          }
        }
    }

    // Play recorded data
    if(isPlaying) {
      elapsedPlaybackTime = millis() - playbackStartTime;

      // Stop playback after passing record time duration
      if (elapsedPlaybackTime >= elapsedRecordTime)
      {
          stopPlayback();
          startListening();
      }
    }
}
