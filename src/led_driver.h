#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>

#define LED_ROWS 5
#define LED_COLS 5

struct led_driver_config
{
  uint8_t i2c_sda_pin;
  uint8_t i2c_scl_pin;
  uint8_t pca_address;
  float pca_frequency_hz;
  uint8_t row_pins[LED_ROWS];
  uint8_t col_channels[LED_COLS];
};

void led_driver_init (const struct led_driver_config *config);
void led_driver_disable_all_rows (void);
void led_driver_enable_row (uint8_t row_index);
void led_driver_set_column_intensity (uint8_t col_index, uint8_t intensity);
void led_driver_apply_row_intensity (uint8_t row_index,
                                     const uint8_t row_values[LED_COLS]);
void led_driver_set_column_binary (uint8_t col_index, uint8_t is_on);
void led_driver_apply_row_binary (uint8_t row_index,
                                  const uint8_t row_values[LED_COLS]);

#endif /* LED_DRIVER_H */