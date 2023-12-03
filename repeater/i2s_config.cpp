/*
 * I2S Configuration
 *
 * Configuration and initialization of the I2S (Inter-IC Sound) interface
 * for both microphone (MIC) and speaker (SPK) modes.
 * Provides methods to set up the I2S driver (for either MIC or SPK mode), 
 * handling sample rate, data format, and pin assignments.
 *
 */

#include "i2s_config.h"

I2sConfig::I2sConfig(Printer printer) : 
  printer(printer) {}

void I2sConfig::micMode() {
  init(MODE_MIC);
}

void I2sConfig::speakerMode() {
  init(MODE_SPK);
}

// - - - - - - - - private - - - - - - - - 

void I2sConfig::init(int mode) {
  esp_err_t err = ESP_OK;
  uint32_t sample_rate = 16000;

  i2s_driver_uninstall(I2S_NUMBER);

  i2s_config_t config = initI2sConfigurationStructure(mode, sample_rate);

  err += i2s_driver_install(I2S_NUMBER, &config, 0, NULL);
  
  i2s_pin_config_t pinConfig = initI2sPinConfiguration();

  err += i2s_set_pin(I2S_NUMBER, &pinConfig);
  err += i2s_set_clk(I2S_NUMBER, sample_rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

i2s_config_t I2sConfig::initI2sConfigurationStructure(int mode, uint32_t sample_rate) {
  i2s_config_t config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER),
      .sample_rate = sample_rate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 6,
      .dma_buf_len = 60,
  };

  if (mode == MODE_MIC) {
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  } else {
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.use_apll = false;
    config.tx_desc_auto_clear = true;
  }

  return config;
}

i2s_pin_config_t I2sConfig::initI2sPinConfiguration() {
  i2s_pin_config_t config;

  config.bck_io_num = CONFIG_I2S_BCK_PIN;
  config.ws_io_num = CONFIG_I2S_LRCK_PIN;
  config.data_out_num = CONFIG_I2S_DATA_PIN;
  config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
  config.mck_io_num = GPIO_NUM_0; // Master clock (MCK) GPIO, set to GPIO_NUM_0

  return config;
}