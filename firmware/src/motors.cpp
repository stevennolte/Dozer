#include "motors.h"

#include <Arduino.h>

#include "pins.h"
#include "protocol.h"

static int8_t apply_deadband(int8_t v) {
    if (v > -DRIVE_DEADBAND && v < DRIVE_DEADBAND) {
        return 0;
    }
    return v;
}

static uint8_t to_duty(int8_t speed) {
    int8_t mag = speed < 0 ? -speed : speed;
    if (mag > DRIVE_MAX) {
        mag = DRIVE_MAX;
    }
    return static_cast<uint8_t>((static_cast<uint16_t>(mag) * 255) / DRIVE_MAX);
}

static void set_channel(int8_t speed, uint8_t pwm_fwd, uint8_t pwm_rev) {
    speed = apply_deadband(speed);
    if (speed > DRIVE_MAX) {
        speed = DRIVE_MAX;
    } else if (speed < -DRIVE_MAX) {
        speed = -DRIVE_MAX;
    }

    if (speed == 0) {
        ledcWrite(pwm_fwd, 0);
        ledcWrite(pwm_rev, 0);
        return;
    }

    const uint8_t duty = to_duty(speed);
    if (speed > 0) {
        ledcWrite(pwm_rev, 0);
        ledcWrite(pwm_fwd, duty);
    } else {
        ledcWrite(pwm_fwd, 0);
        ledcWrite(pwm_rev, duty);
    }
}

void motors_begin() {
    ledcSetup(PWM_CH_IN1, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(PWM_CH_IN2, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(PWM_CH_IN3, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(PWM_CH_IN4, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_IN1, PWM_CH_IN1);
    ledcAttachPin(PIN_IN2, PWM_CH_IN2);
    ledcAttachPin(PIN_IN3, PWM_CH_IN3);
    ledcAttachPin(PIN_IN4, PWM_CH_IN4);
    ledcWrite(PWM_CH_IN1, 0);
    ledcWrite(PWM_CH_IN2, 0);
    ledcWrite(PWM_CH_IN3, 0);
    ledcWrite(PWM_CH_IN4, 0);
    Serial.printf("[motors] IN1=%d IN2=%d IN3=%d IN4=%d  PWM %d Hz\n", PIN_IN1,
                  PIN_IN2, PIN_IN3, PIN_IN4, PWM_FREQ_HZ);
}

void motors_set(int8_t left, int8_t right) {
    set_channel(left, PWM_CH_IN1, PWM_CH_IN2);
    set_channel(right, PWM_CH_IN3, PWM_CH_IN4);
}

void motors_set_pins(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
    ledcWrite(PWM_CH_IN1, in1);
    ledcWrite(PWM_CH_IN2, in2);
    ledcWrite(PWM_CH_IN3, in3);
    ledcWrite(PWM_CH_IN4, in4);
}

void motors_coast() {
    motors_set_pins(0, 0, 0, 0);
}
