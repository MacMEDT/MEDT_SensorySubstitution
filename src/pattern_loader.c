#include <Arduino.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pattern_loader.h"




bool
pattern_load_from_json_string (const char *json, struct led_pattern *out_pattern)
{
  return false;
}

void
pattern_set_fallback (struct led_pattern *out_pattern)
{
  return NULL;
}