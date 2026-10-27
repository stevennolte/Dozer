#include <Arduino.h>

#include "ble_control.h"
#include "motors.h"
#include "protocol.h"
#include "wifi_debug.h"

static uint32_t age_ms(uint32_t last) {
    if (last == 0) {
        return 0xFFFFFFFFu;
    }
    return millis() - last;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("[boot] Dozer firmware");
    Serial.println("[boot] board ESP32-S3-DevKitC-1 N8R2");

    motors_begin();
    wifi_debug_begin();
    ble_begin();
}

void loop() {
    static bool logged_disconnect = false;
    static bool logged_failsafe = false;
    static int8_t last_left = 127;
    static int8_t last_right = 127;
    static uint8_t last_pins[4] = {255, 255, 255, 255};

    wifi_debug_loop();

    if (!ble_is_connected()) {
        motors_coast();
        if (!logged_disconnect) {
            Serial.println("[drive] idle (not connected)");
            logged_disconnect = true;
            logged_failsafe = false;
            last_left = 127;
            last_right = 127;
            last_pins[0] = last_pins[1] = last_pins[2] = last_pins[3] = 255;
        }
        delay(10);
        return;
    }
    logged_disconnect = false;

    const uint32_t drive_age = age_ms(ble_last_drive_ms());
    const uint32_t pins_age = age_ms(ble_last_pins_ms());
    const bool pins_live = pins_age <= DRIVE_FAILSAFE_MS;
    const bool drive_live = drive_age <= DRIVE_FAILSAFE_MS;

    if (pins_live && pins_age <= drive_age) {
        logged_failsafe = false;
        uint8_t in1 = 0, in2 = 0, in3 = 0, in4 = 0;
        ble_get_pins(&in1, &in2, &in3, &in4);
        motors_set_pins(in1, in2, in3, in4);
        if (in1 != last_pins[0] || in2 != last_pins[1] || in3 != last_pins[2] ||
            in4 != last_pins[3]) {
            Serial.printf("[pins] IN1=%u IN2=%u IN3=%u IN4=%u\n", in1, in2, in3,
                          in4);
            last_pins[0] = in1;
            last_pins[1] = in2;
            last_pins[2] = in3;
            last_pins[3] = in4;
        }
    } else if (drive_live) {
        logged_failsafe = false;
        int8_t left = 0;
        int8_t right = 0;
        ble_get_command(&left, &right);
        motors_set(left, right);
        if (left != last_left || right != last_right) {
            Serial.printf("[drive] L=%d R=%d\n", left, right);
            last_left = left;
            last_right = right;
        }
    } else {
        motors_coast();
        if (!logged_failsafe) {
            Serial.println("[drive] failsafe coast (no packet)");
            logged_failsafe = true;
            last_left = 127;
            last_right = 127;
            last_pins[0] = last_pins[1] = last_pins[2] = last_pins[3] = 255;
        }
    }

    delay(10);
}
