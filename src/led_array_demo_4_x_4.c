#include <Arduino.h>
#include <Wire.h>

const uint8_t NUM_ROWS = 4;
const uint8_t NUM_COLS = 4;

const int rowPins[NUM_ROWS] = {23, 19, 18, 5};

const uint8_t i2c_scl_pin = 22;
const uint8_t i2c_sda_pin = 21;

const uint8_t pca_addr = 0x40;
const uint8_t pca_mode1 = 0x00;
const uint8_t pca_prescale = 0xFE;
const uint8_t pca_led0_on_l = 0x06;

const uint8_t colChannels[NUM_COLS] = {1, 2, 3, 4};

const float pca_col_freq_hz = 1000.0f;

/* Current displayed pattern */
uint8_t intensities[NUM_ROWS][NUM_COLS] = {
  {255, 0,   0,   0},
  {0,   0,   0,   0},
  {0,   0,   0,   0},
  {0,   0,   0,   0}
};

unsigned long lastPatternChange = 0;
const unsigned long patternIntervalMs = 1500;
uint8_t patternIndex = 0;

const uint32_t colHoldUs = 3000;

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
  if (Wire.available()) return Wire.read();
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
    pca_set_pwm(channel, 0, 4096);
  } else if (duty >= 4095) {
    pca_set_pwm(channel, 4096, 0);
  } else {
    pca_set_pwm(channel, 0, duty);
  }
}

/* ---------- Row / column helpers ---------- */

void disable_all_rows() {
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
  }
}

void disable_all_columns() {
  for (uint8_t c = 0; c < NUM_COLS; c++) {
    pca_set_duty(colChannels[c], 0);
  }
}

void enable_column(uint8_t col) {
  disable_all_columns();
  pca_set_duty(colChannels[col], 4095);
}

void set_rows_for_column(uint8_t col) {
  for (uint8_t row = 0; row < NUM_ROWS; row++) {
    if (intensities[row][col] > 0) {
      digitalWrite(rowPins[row], HIGH);
    } else {
      digitalWrite(rowPins[row], LOW);
    }
  }
}

/* ---------- Pattern loading ---------- */

void load_pattern(uint8_t pattern[NUM_ROWS][NUM_COLS]) {
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      intensities[r][c] = pattern[r][c];
    }
  }
}

void set_pattern_0() {
  // Single top-left pixel
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {255, 0,   0,   0},
    {0,   0,   0,   0},
    {0,   0,   0,   0},
    {0,   0,   0,   0}
  };
  load_pattern(pattern);
}

void set_pattern_1() {
  // Diagonal
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {255, 0,   0,   0},
    {0,   255, 0,   0},
    {0,   0,   255, 0},
    {0,   0,   0,   255}
  };
  load_pattern(pattern);
}

void set_pattern_2() {
  // X shape
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {255, 0,   0,   255},
    {0,   255, 255, 0},
    {0,   255, 255, 0},
    {255, 0,   0,   255}
  };
  load_pattern(pattern);
}

void set_pattern_3() {
  // Border
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {255, 255, 255, 255},
    {255, 0,   0,   255},
    {255, 0,   0,   255},
    {255, 255, 255, 255}
  };
  load_pattern(pattern);
}

void set_pattern_4() {
  // Checkerboard
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {255, 0,   255, 0},
    {0,   255, 0,   255},
    {255, 0,   255, 0},
    {0,   255, 0,   255}
  };
  load_pattern(pattern);
}

void set_pattern_5() {
  // Plus sign
  uint8_t pattern[NUM_ROWS][NUM_COLS] = {
    {0,   255, 255, 0},
    {255, 255, 255, 255},
    {255, 255, 255, 255},
    {0,   255, 255, 0}
  };
  load_pattern(pattern);
}

void update_pattern() {
  unsigned long now = millis();
  if (now - lastPatternChange < patternIntervalMs) return;

  lastPatternChange = now;
  patternIndex = (patternIndex + 1) % 6;

  Serial.print("Pattern index: ");
  Serial.println(patternIndex);

  switch (patternIndex) {
    case 0: set_pattern_0(); break;
    case 1: set_pattern_1(); break;
    case 2: set_pattern_2(); break;
    case 3: set_pattern_3(); break;
    case 4: set_pattern_4(); break;
    case 5: set_pattern_5(); break;
  }
}

/* ---------- Arduino ---------- */

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
  }

  disable_all_rows();

  Wire.begin(i2c_sda_pin, i2c_scl_pin);
  delay(10);
  pca_set_pwm_freq(pca_col_freq_hz);

  disable_all_columns();

  set_pattern_0();
}

void loop() {
  update_pattern();

  for (uint8_t col = 0; col < NUM_COLS; col++) {
    disable_all_columns();
    set_rows_for_column(col);
    enable_column(col);
    delayMicroseconds(colHoldUs);
  }
}
