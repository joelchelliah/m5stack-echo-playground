#ifndef I2S_CONFIG_H
#define I2S_CONFIG_H

#include <driver/i2s.h>
#include "printer.h"

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define I2S_NUMBER I2S_NUM_0

class I2sConfig {
public:
  I2sConfig(Printer printer);
  void micMode();
  void speakerMode();

private:
  Printer printer;

  static const int MODE_MIC = 0;
  static const int MODE_SPK = 1;

  void init(int mode);
  i2s_config_t initI2sConfigurationStructure(int mode, uint32_t sample_rate);
  i2s_pin_config_t initI2sPinConfiguration();
};

#endif