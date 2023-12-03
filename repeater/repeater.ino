#include <driver/i2s.h>
#include <M5Atom.h>

#include "printer.h"
#include "i2s_config.h"
#include "voice_box.h"


// Sound recording, analysis and calibration
#define DATA_SIZE 1024
#define MAX_RECORDING_DURATION 2000
#define MAX_SILENCE_DURATION 1000

Printer printer(Serial);
I2sConfig i2sConfig(printer);
VoiceBox voiceBox(DATA_SIZE, printer);

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

// Playback control variables
bool isPlaying = false;
unsigned long playbackStartTime = 0;
unsigned long elapsedPlaybackTime = 0;

void startListening() {
    printer.msg("- Listening started -");
    isListening = true;
    isRecording = false;
    isPlaying = false;
    data_offset = 0;

    i2sConfig.micMode();
}

void startRecording() {
    if(!isListening) {
      throw std::runtime_error("Tried to record when not listening!");
    }

    printer.msg("- Recording started -");
    isRecording = true;
    elapsedRecordTime = 0;
    recordStartTime = millis();
}

void stopListeningAndRecordingAndStartPlayback() {
    printer.keyVal("Recording stopped, ms", elapsedRecordTime);
    isListening = false;
    isRecording = false;

    i2sConfig.speakerMode();

    printer.msg("- Playback started -");
    isPlaying = true;

    elapsedPlaybackTime = 0;
    playbackStartTime = millis();

    size_t bytes_written;
    i2s_write(I2S_NUMBER, microphonedata0, data_offset, &bytes_written, portMAX_DELAY);
}

void stopPlayback() {
    printer.keyVal("Playback stopped, ms", elapsedPlaybackTime);
    isPlaying = false;
}

void showPausedMessage() {
  printer.msg(" - - - - Paused - - - - -");
  printer.msg(" - - ( calibrating ) - - ");
  printer.msg("- Press button to start -");
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
          voiceBox.collectSoundLevelCalibrationData(adcBuffer);
          return;
        } else {
          voiceBox.setSoundLevelBasedOnCalibrationData();
        }

        bool wasSoundDetected = voiceBox.wasSoundAboveSoundThreshold(adcBuffer);

        // Decide if recording-mode should be triggered
        if(!isRecording) {
          if (wasSoundDetected) {
            startRecording();
          } else {
            printer.msg("Still listening...");
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
            printer.keyVal("Silence, ms", elapsedSilenceTime);

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
