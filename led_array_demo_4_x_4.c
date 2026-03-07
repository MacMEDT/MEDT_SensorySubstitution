#include <Arduino.h>
#include <Wire.h>

/* ---------- Matrix size ---------- */
const uint8_t NUM_ROWS = 4;
const uint8_t NUM_COLS = 4;

/* ---------- Row GPIO pins (adjust to your wiring) ---------- */
const int rowPins[NUM_ROWS] = {
  23,   // row 1
  19,   // row 2
  18,  // row 3
  5   // row 4
};

/* ---------- I2C pins ---------- */
const uint8_t i2c_scl_pin = 22;
const uint8_t i2c_sda_pin = 21;

/* ---------- PCA9685 address and registers ---------- */
const uint8_t pca_addr = 0x40;
const uint8_t pca_mode1 = 0x00;
const uint8_t pca_prescale = 0xFE;
const uint8_t pca_led0_on_l = 0x06;

/* ---------- PCA channels used as column sinks ---------- */
const uint8_t colChannels[NUM_COLS] = {
  1, 2, 3, 4
};

const float pca_col_freq_hz = 1000.0f;

/* ---------- 4x4 intensity table ---------- */
/* 0 = off, 255 = max brightness */
uint8_t intensities[NUM_ROWS][NUM_COLS] = {
  {0,   0,   0, 0},
  {0,   0,   0, 0},
  {0,   0,   0, 0},
  {0,   0,   0, 0}
};

const uint16_t frame_rate_hz = 60;
const uint32_t frame_us = 1000000UL / frame_rate_hz;
const uint32_t row_hold_us = frame_us / NUM_ROWS;

unsigned long last_pattern_update_ms = 0;
const uint16_t pattern_interval_ms = 1500;

/* ---------- PCA helpers ---------- */

void pca_write_8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(pca_addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t pca_read_8(uint8_t reg) {
  Wire.beginTransmission(pca_addr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(pca_addr, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

void pca_set_pwm_freq(float freq_hz) {
  float prescale_val = 25000000.0f / (4096.0f * freq_hz) - 1.0f;
  uint8_t prescale = (uint8_t)(prescale_val + 0.5f);

  uint8_t old_mode = pca_read_8(pca_mode1);
  uint8_t sleep_mode = (old_mode & 0x7F) | 0x10;

  pca_write_8(pca_mode1, sleep_mode);
  pca_write_8(pca_prescale, prescale);
  pca_write_8(pca_mode1, old_mode);
  delay(5);
  pca_write_8(pca_mode1, old_mode | 0xA1);
}

void pca_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count) {
  uint8_t reg = pca_led0_on_l + 4 * channel;
  Wire.beginTransmission(pca_addr);
  Wire.write(reg);
  Wire.write(on_count & 0xFF);
  Wire.write(on_count >> 8);
  Wire.write(off_count & 0xFF);
  Wire.write(off_count >> 8);
  Wire.endTransmission();
}

void pca_set_duty(uint8_t channel, uint16_t duty) {
  if (duty == 0) {
    pca_set_pwm(channel, 0, 4096);      // fully off
  } else if (duty >= 4095) {
    pca_set_pwm(channel, 4096, 0);      // fully on
  } else {
    pca_set_pwm(channel, 0, duty);
  }
}

uint16_t intensity_to_pwm(uint8_t intensity) {
  return (uint16_t)((intensity / 255.0f) * 4095.0f + 0.5f);
}

/* ---------- Column update for one active row ---------- */
void apply_row_to_columns(uint8_t row) {
  for (uint8_t col = 0; col < NUM_COLS; col++) {
    uint16_t pwm = intensity_to_pwm(intensities[row][col]);
    pca_set_duty(colChannels[col], pwm);
  }
}

/* ---------- Row control ---------- */
void disable_all_rows() {
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
  }
}

void enable_row(uint8_t row) {
  disable_all_rows();
  if (row < NUM_ROWS) {
    digitalWrite(rowPins[row], HIGH);
  }
}

/* ---------- Demo patterns ---------- */
void set_pattern_0() {
  // X pattern
  uint8_t temp[NUM_ROWS][NUM_COLS] = {
    {0,   0,   0, 0},
    {0,   0,   0, 0},
    {0,   0,   0, 0},
    {0,   0,   0, 0},
  };

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      intensities[r][c] = temp[r][c];
    }
  }
}

void set_pattern_1() {
  // Border
  uint8_t temp[NUM_ROWS][NUM_COLS] = {
    {255, 255, 255, 255},
    {255,   0,   0, 255},
    {255,   0,   0, 255},
    {255, 255, 255, 255}
  };

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      intensities[r][c] = temp[r][c];
    }
  }
}

void set_pattern_2() {
  // Checkerboard
  uint8_t temp[NUM_ROWS][NUM_COLS] = {
    {255,   0, 255,   0},
    {  0, 255,   0, 255},
    {255,   0, 255,   0},
    {  0, 255,   0, 255}
  };

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      intensities[r][c] = temp[r][c];
    }
  }
}

void set_pattern_3() {
  // Gradient-ish demo
  uint8_t temp[NUM_ROWS][NUM_COLS] = {
    { 32,  64, 128, 255},
    { 64, 128, 255, 128},
    {128, 255, 128,  64},
    {255, 128,  64,  32}
  };

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      intensities[r][c] = temp[r][c];
    }
  }
}

void update_demo_pattern() {
  unsigned long now = millis();
  if (now - last_pattern_update_ms < pattern_interval_ms) {
    return;
  }

  last_pattern_update_ms = now;

  static uint8_t state = 0;
  state = (state + 1) % 4;

  switch (state) {
    case 0:
      set_pattern_0();
      break;
    case 1:
      set_pattern_1();
      break;
    case 2:
      set_pattern_2();
      break;
    case 3:
      set_pattern_3();
      break;
  }
}

/* ---------- Arduino entry points ---------- */
void setup() {
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
  }
  disable_all_rows();

  Wire.begin(i2c_sda_pin, i2c_scl_pin);
  delay(10);

  pca_set_pwm_freq(pca_col_freq_hz);

  // Start with all column channels off
  for (uint8_t col = 0; col < NUM_COLS; col++) {
    pca_set_duty(colChannels[col], 0);
  }
}

void loop() {
  update_demo_pattern();

  for (uint8_t row = 0; row < NUM_ROWS; row++) {
    disable_all_rows();          // prevent ghosting
    apply_row_to_columns(row);   // load this row's 4 column brightness values
    enable_row(row);             // turn on only this row
    delayMicroseconds(row_hold_us * 30);
  }
}
