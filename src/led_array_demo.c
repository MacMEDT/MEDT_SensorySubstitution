#include <Arduino.h>
#include <Wire.h>

/* Use raw GPIO numbers; adjust to match your wiring.  */
const int row0_pin = 5;
const int row1_pin = 4;

const uint8_t i2c_scl_pin = 14;
const uint8_t i2c_sda_pin = 12;

/* PCA9685 address and registers.  */
const uint8_t pca_addr = 0x40;
const uint8_t pca_mode1 = 0x00;
const uint8_t pca_prescale = 0xfe;
const uint8_t pca_led0_on_l = 0x06;

/* PCA channels used as column sinks.  */
const uint8_t col_ch_0 = 1;
const uint8_t col_ch_1 = 2;

const float pca_col_freq_hz = 1000.0f;

/* 2x2 intensities table.  */
uint8_t intensities[2][2] = {
  { 32, 128 },
  { 200, 255 }
};

uint16_t col_pwm[2] = { 0, 0 };

const uint16_t frame_rate_hz = 50;
const uint32_t frame_us = 1000000UL / frame_rate_hz;
const uint32_t row_hold_us = frame_us / 2;

unsigned long last_pattern_update_ms = 0;
const uint16_t pattern_interval_ms = 1500;

/* PCA helpers.  */

void
pca_write_8 (uint8_t reg, uint8_t val)
{
  Wire.beginTransmission (pca_addr);
  Wire.write (reg);
  Wire.write (val);
  Wire.endTransmission ();
}

uint8_t
pca_read_8 (uint8_t reg)
{
  Wire.beginTransmission (pca_addr);
  Wire.write (reg);
  Wire.endTransmission ();
  Wire.requestFrom (pca_addr, (uint8_t) 1);
  if (Wire.available ())
    return Wire.read ();
  return 0;
}

void
pca_set_pwm_freq (float freq_hz)
{
  float prescale_val = 25000000.0f / (4096.0f * freq_hz) - 1.0f;
  uint8_t prescale = (uint8_t) (prescale_val + 0.5f);

  uint8_t old_mode = pca_read_8 (pca_mode1);
  uint8_t sleep_mode = (old_mode & 0x7f) | 0x10;
  pca_write_8 (pca_mode1, sleep_mode);
  pca_write_8 (pca_prescale, prescale);
  pca_write_8 (pca_mode1, old_mode);
  delay (5);
  pca_write_8 (pca_mode1, old_mode | 0xa1);
}

void
pca_set_pwm (uint8_t channel, uint16_t on_count, uint16_t off_count)
{
  uint8_t reg = pca_led0_on_l + 4 * channel;
  Wire.beginTransmission (pca_addr);
  Wire.write (reg);
  Wire.write (on_count & 0xff);
  Wire.write (on_count >> 8);
  Wire.write (off_count & 0xff);
  Wire.write (off_count >> 8);
  Wire.endTransmission ();
}

void
pca_set_duty (uint8_t channel, uint16_t duty)
{
  if (duty == 0)
    pca_set_pwm (channel, 0, 4096);
  else if (duty >= 4095)
    pca_set_pwm (channel, 4096, 0);
  else
    pca_set_pwm (channel, 0, duty);
}

/* Intensity to column PWM conversion.  */

uint16_t
intensity_to_pwm (uint8_t intensity)
{
  return (uint16_t) ((intensity / 255.0f) * 4095.0f + 0.5f);
}

void
compute_column_pwm_from_intensities (void)
{
  for (uint8_t col = 0; col < 2; col++)
    {
      uint16_t sum = 0;
      for (uint8_t row = 0; row < 2; row++)
        sum += intensities[row][col];
      uint8_t mean = sum / 2;
      col_pwm[col] = intensity_to_pwm (mean);
    }
}

void
apply_column_pwm_to_pca (void)
{
  pca_set_duty (col_ch_0, col_pwm[0]);
  pca_set_duty (col_ch_1, col_pwm[1]);
}

/* Row control functions.  */

void
disable_all_rows (void)
{
  digitalWrite (row0_pin, LOW);
  digitalWrite (row1_pin, LOW);
}

void
enable_row (uint8_t row)
{
  disable_all_rows ();
  if (row == 0)
    digitalWrite (row0_pin, HIGH);
  else if (row == 1)
    digitalWrite (row1_pin, HIGH);
}

/* Pattern update logic.  */

void
update_demo_pattern (void)
{
  unsigned long now = millis ();
  if (now - last_pattern_update_ms < pattern_interval_ms)
    return;
  last_pattern_update_ms = now;

  static uint8_t state = 0;
  state = (state + 1) % 4;

  switch (state)
    {
    case 0:
      intensities[0][0] = 32;  intensities[0][1] = 32;
      intensities[1][0] = 32;  intensities[1][1] = 32;
      break;
    case 1:
      intensities[0][0] = 32;  intensities[0][1] = 128;
      intensities[1][0] = 200; intensities[1][1] = 255;
      break;
    case 2:
      intensities[0][0] = 255; intensities[0][1] = 32;
      intensities[1][0] = 32;  intensities[1][1] = 255;
      break;
    case 3:
      intensities[0][0] = 255; intensities[0][1] = 255;
      intensities[1][0] = 255; intensities[1][1] = 255;
      break;
    }

  compute_column_pwm_from_intensities ();
  apply_column_pwm_to_pca ();
}

/* Standard Arduino entry points.  */

void
setup (void)
{
  pinMode (row0_pin, OUTPUT);
  pinMode (row1_pin, OUTPUT);
  disable_all_rows ();

  Wire.begin (i2c_sda_pin, i2c_scl_pin);
  delay (10);
  pca_set_pwm_freq (pca_col_freq_hz);

  compute_column_pwm_from_intensities ();
  apply_column_pwm_to_pca ();
}

void
loop (void)
{
  update_demo_pattern ();
  for (uint8_t row = 0; row < 2; row++)
    {
      enable_row (row);
      delayMicroseconds (row_hold_us);
    }
}
