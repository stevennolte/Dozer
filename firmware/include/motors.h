#pragma once

#include <stdint.h>

void motors_begin();
void motors_set(int8_t left, int8_t right);
void motors_set_pins(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);
void motors_coast();
