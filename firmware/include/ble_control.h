#pragma once

#include <stdint.h>

void ble_begin();
bool ble_is_connected();
uint32_t ble_last_drive_ms();
uint32_t ble_last_pins_ms();
void ble_get_command(int8_t *left, int8_t *right);
void ble_get_pins(uint8_t *in1, uint8_t *in2, uint8_t *in3, uint8_t *in4);
