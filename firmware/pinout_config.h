/**
 * MARUK (SOCCER) - Hardware Pinout Configuration
 * Compatible with ESP32-WROOM-32 DevKit
 */

#ifndef PINOUT_CONFIG_H
#define PINOUT_CONFIG_H

// ==========================================
// MOTOR DRIVER PINS (e.g. 2x L298N or 4x BTS7960 / MDD10A)
// Each wheel uses 1 PWM pin and 1 DIR pin (or 2 PWM pins for H-bridge)
// ==========================================

// Front-Left Motor
#define PIN_FL_PWM  13
#define PIN_FL_DIR  12

// Front-Right Motor
#define PIN_FR_PWM  14
#define PIN_FR_DIR  27

// Rear-Left Motor
#define PIN_RL_PWM  26
#define PIN_RL_DIR  25

// Rear-Right Motor
#define PIN_RR_PWM  33
#define PIN_RR_DIR  32

// ==========================================
// MECHANISM PINS
// ==========================================
// Dribbler Roller (DC motor or ESC)
#define PIN_DRIBBLER_PWM  19
#define PIN_DRIBBLER_DIR  18

// Kicker Actuator (MOSFET module triggering a 12V/24V solenoid)
#define PIN_KICKER_SOLENOID 23

// Onboard Status LED
#define PIN_STATUS_LED    2

// ==========================================
// PWM TIMER SETTINGS (ESP32 LEDC)
// ==========================================
#define PWM_FREQ_HZ       15000  // 15 kHz (Silent motor frequency)
#define PWM_RESOLUTION_BITS 8    // 8-bit (0 - 255)

#endif // PINOUT_CONFIG_H
