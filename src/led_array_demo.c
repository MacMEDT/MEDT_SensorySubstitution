#include <Arduino.h>
#include <Wire.h>
#include "led_driver.h"

static const uint8_t esp32_i2c_slave_address = 0x28;
static const uint8_t pi_i2c_sda_pin = 21;
static const uint8_t pi_i2c_scl_pin = 22;
static const uint32_t i2c_bus_frequency_hz = 100000;

static const int rows = LED_ROWS;
static const int cols = LED_COLS;
static const int pwm_levels = 5;
static const uint16_t refresh_hz = 60;
static const uint32_t frame_us = 1000000UL / refresh_hz;
static const uint32_t slice_us = frame_us / (rows * pwm_levels);
static const int packet_len = 27;

static volatile bool new_frame_ready = false;
static volatile uint8_t back_frame[LED_ROWS][LED_COLS];
static uint8_t front_frame[LED_ROWS][LED_COLS];
static volatile uint8_t current_mode = 0;

static uint8_t
checksum_xor (const uint8_t *bytes, uint8_t len)
{
  uint8_t checksum;

  checksum = 0;
  for (uint8_t index = 0; index < len; index++)
    checksum ^= bytes[index];

  return checksum;
}

static void
disableAllRows (void)
{
  led_driver_disable_all_rows ();
}

static void
enableRow (int row)
{
  led_driver_enable_row ((uint8_t) row);
}

static void
writeColumns (uint8_t col_bits)
{
  disableAllRows ();

  for (int c = 0; c < cols; c++)
    {
      uint8_t is_on;

      is_on = (uint8_t) ((col_bits >> c) & 0x01);
      led_driver_set_column_binary ((uint8_t) c, is_on);
    }
}

static uint8_t
intensity_to_level (uint8_t intensity)
{
  return (uint8_t) ((intensity * pwm_levels) / 256);
}

static uint8_t
computeColBitsRowSlice (uint8_t frame[LED_ROWS][LED_COLS], int r, int s)
{
  uint8_t col_bits;

  col_bits = 0;
  for (int c = 0; c < cols; c++)
    {
      uint8_t level;

      level = intensity_to_level (frame[r][c]);
      if (level > s)
        col_bits |= (uint8_t) (1U << c);
    }

  return col_bits;
}

static void
swapBuffersIfReady (void)
{
  if (!new_frame_ready)
    return;

  noInterrupts ();
  new_frame_ready = false;
  interrupts ();

  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
      front_frame[r][c] = back_frame[r][c];
}

static void
onI2CReceive (int howMany)
{
  uint8_t buffer[packet_len];
  int index;
  uint8_t mode;
  uint8_t cs_pi;
  uint8_t cs_calc;
  int k;

  if (howMany < 1)
    return;

  index = 0;
  while (Wire.available () && index < packet_len)
    buffer[index++] = (uint8_t) Wire.read ();

  if (index != packet_len)
    {
      while (Wire.available ())
        (void) Wire.read ();
      return;
    }

  mode = buffer[0];
  cs_pi = buffer[26];

  cs_calc = 0;
  for (int i = 0; i < 26; i++)
    cs_calc ^= buffer[i];

  if (cs_calc != cs_pi)
    return;

  k = 1;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
      back_frame[r][c] = buffer[k++];

  current_mode = mode;
  new_frame_ready = true;
}

static const struct led_driver_config driver_config = {
  .i2c_sda_pin = 12,
  .i2c_scl_pin = 14,
  .pca_address = 0x40,
  .pca_frequency_hz = 1000.0f,
  .row_pins = { 5, 4, 3, 2, 1 },
  .col_channels = { 0, 1, 2, 3, 4 }
};

void
setup (void)
{
  Serial.begin (115200);

  Wire.begin (esp32_i2c_slave_address,
              pi_i2c_sda_pin,
              pi_i2c_scl_pin,
              i2c_bus_frequency_hz);
  Wire.onReceive (onI2CReceive);

  led_driver_init (&driver_config);
  memset ((void *) back_frame, 0, sizeof back_frame);
  memset (front_frame, 0, sizeof front_frame);
  Serial.println ("Ready for Pi frames: [mode][25 bytes][checksum].");
}

void
loop (void)
{
  swapBuffersIfReady ();

  for (int r = 0; r < rows; r++)
    {
      enableRow (r);

      for (int s = 0; s < pwm_levels; s++)
        {
          uint8_t col_bits;

          col_bits = computeColBitsRowSlice (front_frame, r, s);
          writeColumns (col_bits);
          enableRow (r);
          delayMicroseconds (slice_us);
        }

      disableAllRows ();
    }
}
