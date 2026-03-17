#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const uint8_t NUM_ROWS = 4;
const uint8_t NUM_COLS = 4;

const int rowPins[NUM_ROWS] = {23, 19, 18, 5};

const uint8_t i2c_scl_pin = 22;
const uint8_t i2c_sda_pin = 21;

const uint8_t pca_addr = 0x40;

const uint8_t colChannels[NUM_COLS] = {1, 2, 3, 4};
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(pca_addr, Wire);

const float pca_col_freq_hz = 1000.0f;

/* Current displayed pattern */
uint8_t intensities[NUM_ROWS][NUM_COLS] = {
  {255, 0,   0,   0},
  {0,   0,   0,   0},
  {0,   0,   0,   0},
  {0,   0,   0,   0}
};

unsigned long lastPatternChange = 0;
unsigned long patternIntervalMs = 10000;
const uint8_t NUM_MODES = 6;
uint8_t currentMode = 0;

const uint32_t colHoldUs = 3000;

/* ---------- PCA helpers ---------- */

void pca_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count) {
  pca.setPWM(channel, on_count, off_count);
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
    pca_set_duty(colChannels[c], 4095);
  }
}

void enable_column(uint8_t col) {
  disable_all_columns();
  pca_set_duty(colChannels[col], 0);
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

void apply_mode(uint8_t mode) {
  switch (mode) {
    case 0: set_pattern_0(); break;
    case 1: set_pattern_1(); break;
    case 2: set_pattern_2(); break;
    case 3: set_pattern_3(); break;
    case 4: set_pattern_4(); break;
    case 5: set_pattern_5(); break;
    default: set_pattern_0(); break;
  }
}

void update_pattern() {
  unsigned long now = millis();
  if (now - lastPatternChange < patternIntervalMs) return;

  lastPatternChange = now;
  uint8_t nextMode = currentMode;
  while (nextMode == currentMode) {
    nextMode = (uint8_t)random(NUM_MODES);
  }
  currentMode = nextMode;
  patternIntervalMs = (unsigned long)random(10000, 15001);

  Serial.print("Mode: ");
  Serial.print(currentMode);
  Serial.print(" | Next switch in ms: ");
  Serial.println(patternIntervalMs);

  apply_mode(currentMode);
}

/* ---------- Arduino ---------- */

void setup() {
  Serial.begin(115200);
  randomSeed((unsigned long)micros());

  for (uint8_t i = 0; i < NUM_ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
  }

  disable_all_rows();

  Wire.begin(i2c_sda_pin, i2c_scl_pin);
  delay(10);
  pca.begin();
  pca.setPWMFreq(pca_col_freq_hz);

  disable_all_columns();

  currentMode = (uint8_t)random(NUM_MODES);
  patternIntervalMs = (unsigned long)random(10000, 15001);
  apply_mode(currentMode);
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
