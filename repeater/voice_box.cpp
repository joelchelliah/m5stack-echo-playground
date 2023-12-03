#include "voice_box.h"

VoiceBox::VoiceBox(uint16_t dataSize, Printer printer) : 
  printer(printer),
  numSoundSamples(dataSize / sizeof(int16_t)),
  isSoundLevelCalibrated(false), 
  averageSoundLevelCalibrationData(0), 
  averageSoundLevelCalibrationCount(0), 
  dynamicSoundThreshold(0) {}

void VoiceBox::collectSoundLevelCalibrationData(int16_t *buffer) {
  resetCalibrationData();

  int soundLevel;
  int startSample = 10; // Skip the first x samples because of mysterious noise?

  for (int n = startSample; n < numSoundSamples; n++) {
    soundLevel = buffer[n] * GAIN_FACTOR;
    averageSoundLevelCalibrationData += soundLevel;
    averageSoundLevelCalibrationCount++;
  }
}

void VoiceBox::setSoundLevelBasedOnCalibrationData() {
  if(isSoundLevelCalibrated) {
    return;
  }

  isSoundLevelCalibrated = true;
  int averageSoundLevel = 0;

  if (averageSoundLevelCalibrationCount > 0) {
    averageSoundLevel = averageSoundLevelCalibrationData / averageSoundLevelCalibrationCount;
  }
  dynamicSoundThreshold = DYNAMIC_SOUND_THRESHOLD_MULTIPLIER * averageSoundLevel;

  if(dynamicSoundThreshold < MIN_DYNAMIC_SOUND_THRESHOLD) {
    dynamicSoundThreshold = MIN_DYNAMIC_SOUND_THRESHOLD;
  }

  printer.keyVal("Average Sound Level", averageSoundLevel);
  printer.keyVal("Dynamic Threshold", dynamicSoundThreshold);
}

bool VoiceBox::wasSoundAboveSoundThreshold(int16_t *buffer) {
  int soundLevel;
  int startSample = 5; //Skip the first x samples because of mysterious noise?
  int soundsAboveThreshold[numSoundSamples];
  int numSoundSamplesAboveThreshold = 0;


  for (int n = startSample; n < numSoundSamples; n++) {
    soundLevel = buffer[n] * GAIN_FACTOR;

    if (soundLevel > dynamicSoundThreshold) {
      printer.keyVal("Sound level", soundLevel);
      soundsAboveThreshold[numSoundSamplesAboveThreshold++] = soundLevel;
    }
  }

  if (numSoundSamplesAboveThreshold > 5) {
    printer.msg("- Above sound threshold -");
  }

  return numSoundSamplesAboveThreshold > 5;
}

// - - - - - - - - private - - - - - - - - 

void VoiceBox::resetCalibrationData() {
  isSoundLevelCalibrated = false;
  averageSoundLevelCalibrationData = 0;
  averageSoundLevelCalibrationCount = 0;
}
