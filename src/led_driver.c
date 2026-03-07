#include <Arduino.h>
#include <Wire.h>
#include "led_driver.h"

#if defined (ARDUINO_ARCH_ESP32)
static TwoWire *pca_wire = &Wire1;
#else
static TwoWire *pca_wire = &Wire;
#endif

static const uint8_t pca_mode1 = 0x00;
static const uint8_t pca_prescale = 0xfe;
static const uint8_t pca_led0_on_l = 0x06;

static struct led_driver_config active_config;

static void
pca_write_8 (uint8_t reg, uint8_t val)
{
  pca_wire->beginTransmission (active_config.pca_address);
  pca_wire->write (reg);
  pca_wire->write (val);
  pca_wire->endTransmission ();
}

static uint8_t
pca_read_8 (uint8_t reg)
{
  pca_wire->beginTransmission (active_config.pca_address);
  pca_wire->write (reg);
  pca_wire->endTransmission ();

  pca_wire->requestFrom (active_config.pca_address, (uint8_t) 1);
  if (pca_wire->available ())
    return pca_wire->read ();

  return 0;
}

static void
pca_set_pwm (uint8_t channel, uint16_t on_count, uint16_t off_count)
{
  uint8_t reg;

  reg = (uint8_t) (pca_led0_on_l + 4 * channel);
  pca_wire->beginTransmission (active_config.pca_address);
  pca_wire->write (reg);
  pca_wire->write ((uint8_t) (on_count & 0xff));
  pca_wire->write ((uint8_t) (on_count >> 8));
  pca_wire->write ((uint8_t) (off_count & 0xff));
  pca_wire->write ((uint8_t) (off_count >> 8));
  pca_wire->endTransmission ();
}

static void
pca_set_duty (uint8_t channel, uint16_t duty)
{
  if (duty == 0)
    pca_set_pwm (channel, 0, 4096);
  else if (duty >= 4095)
    pca_set_pwm (channel, 4096, 0);
  else
    pca_set_pwm (channel, 0, duty);
}

static void
pca_set_pwm_freq (float freq_hz)
{
  float prescale_val;
  uint8_t prescale;
  uint8_t old_mode;
  uint8_t sleep_mode;

  prescale_val = 25000000.0f / (4096.0f * freq_hz) - 1.0f;
  prescale = (uint8_t) (prescale_val + 0.5f);

  old_mode = pca_read_8 (pca_mode1);
  sleep_mode = (uint8_t) ((old_mode & 0x7f) | 0x10);

  pca_write_8 (pca_mode1, sleep_mode);
  pca_write_8 (pca_prescale, prescale);
  pca_write_8 (pca_mode1, old_mode);
  delay (5);
  pca_write_8 (pca_mode1, (uint8_t) (old_mode | 0xa1));
}

void
led_driver_disable_all_rows (void)
{
  for (uint8_t row = 0; row < LED_ROWS; row++)
    digitalWrite (active_config.row_pins[row], LOW);
}

void
led_driver_enable_row (uint8_t row_index)
{
  led_driver_disable_all_rows ();
  if (row_index < LED_ROWS)
    digitalWrite (active_config.row_pins[row_index], HIGH);
}

void
led_driver_set_column_intensity (uint8_t col_index, uint8_t intensity)
{
  uint16_t duty;

  if (col_index >= LED_COLS)
    return;

  duty = (uint16_t) (((uint32_t) intensity * 4095U) / 255U);
  pca_set_duty (active_config.col_channels[col_index], duty);
}

void
led_driver_apply_row_intensity (uint8_t row_index,
                                const uint8_t row_values[LED_COLS])
{
  for (uint8_t col = 0; col < LED_COLS; col++)
    led_driver_set_column_intensity (col, row_values[col]);

  led_driver_enable_row (row_index);
}

void
led_driver_set_column_binary (uint8_t col_index, uint8_t is_on)
{
  uint8_t intensity;

  intensity = is_on ? 255 : 0;
  led_driver_set_column_intensity (col_index, intensity);
}

void
led_driver_apply_row_binary (uint8_t row_index,
                             const uint8_t row_values[LED_COLS])
{
  uint8_t row_binary[LED_COLS];

  for (uint8_t col = 0; col < LED_COLS; col++)
    row_binary[col] = row_values[col] ? 255 : 0;

  led_driver_apply_row_intensity (row_index, row_binary);
}

void
led_driver_init (const struct led_driver_config *config)
{
  active_config = *config;

  for (uint8_t row = 0; row < LED_ROWS; row++)
    {
      pinMode (active_config.row_pins[row], OUTPUT);
      digitalWrite (active_config.row_pins[row], LOW);
    }

#if defined (ARDUINO_ARCH_ESP32)
  pca_wire->begin (active_config.i2c_sda_pin,
                   active_config.i2c_scl_pin,
                   100000UL);
#else
  pca_wire->begin ();
#endif

  delay (10);
  pca_set_pwm_freq (active_config.pca_frequency_hz);

  for (uint8_t col = 0; col < LED_COLS; col++)
    led_driver_set_column_binary (col, 0);
}