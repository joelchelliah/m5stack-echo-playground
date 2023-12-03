#ifndef VOICE_BOX_H
#define VOICE_BOX_H

#include <inttypes.h>
#include "printer.h"

class VoiceBox {
public:
  VoiceBox(uint16_t dataSize, Printer printer);
  void collectSoundLevelCalibrationData(int16_t *buffer);
  void setSoundLevelBasedOnCalibrationData();
  bool wasSoundAboveSoundThreshold(int16_t *buffer);

private:
  Printer printer;

  static const int GAIN_FACTOR = 1;
  int numSoundSamples;

  // - - - - - Sound level threshold and calibration - - - - -
  static const int DYNAMIC_SOUND_THRESHOLD_MULTIPLIER = 2;
  static const int MIN_DYNAMIC_SOUND_THRESHOLD = 2500;
  int dynamicSoundThreshold;
  int averageSoundLevelCalibrationData;
  int averageSoundLevelCalibrationCount;
  bool isSoundLevelCalibrated;
  void resetCalibrationData();
  int calculateDynamicSoundThreshold();
};

#endif
