#ifndef PATTERN_LOADER_H
#define PATTERN_LOADER_H

#include <Arduino.h>
#include <stdbool.h>
#include "led_driver.h"

#define PATTERN_NAME_MAX_LEN 32
#define PATTERN_MAX_FRAMES 64

struct led_pattern
{
  char name[PATTERN_NAME_MAX_LEN];
  uint16_t frame_count;
  uint8_t frames[PATTERN_MAX_FRAMES][LED_ROWS][LED_COLS];
};

bool pattern_load_from_json_string (const char *json,
                                    struct led_pattern *out_pattern);
void pattern_set_fallback (struct led_pattern *out_pattern);

#endif /* PATTERN_LOADER_H */