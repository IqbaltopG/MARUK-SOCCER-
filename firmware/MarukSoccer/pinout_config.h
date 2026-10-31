/**
 * MARUK (SOCCER) - Hardware Pinout Configuration (2WD Edition)
 * Conforms 100% to MIT-WEEK 2026 Car Soccer Rules.
 * 
 * Hardware:
 *   - ESP32-WROOM-32
 *   - 1x Driver L298N (Mengendalikan 2 Motor Gearbox Kuning)
 *   - 1x Receiver Drone (Port Serial2 RX / GPIO 16)
 *   - 1x LM2596 (Step-down baterai ke 5.0V stabil)
 */

#ifndef PINOUT_CONFIG_H
#define PINOUT_CONFIG_H

// ==========================================
// 1x L298N DUAL H-BRIDGE (2 MOTOR KUNING)
// Pasang jumper 5V pada ENA dan ENB di modul L298N
// ==========================================

// Motor Kiri (OUT1 & OUT2 L298N)
#define PIN_LEFT_IN1    13   // Forward PWM
#define PIN_LEFT_IN2    12   // Reverse PWM

// Motor Kanan (OUT3 & OUT4 L298N)
#define PIN_RIGHT_IN3   14   // Forward PWM
#define PIN_RIGHT_IN4   27   // Reverse PWM

// ==========================================
// DRONE RC RECEIVER (SERIAL INTERFACE)
// Cukup 1 kabel data dari receiver ke pin RX2
// ==========================================
#define PIN_RC_RX       16   // GPIO 16 (ESP32 Hardware Serial2 RX)

// Onboard Status LED
#define PIN_STATUS_LED  2

// ==========================================
// PWM TIMER SETTINGS (ESP32 LEDC)
// ==========================================
#define PWM_FREQ_HZ         10000  // 10 kHz (Halus untuk motor kuning TT)
#define PWM_RESOLUTION_BITS 8      // 8-bit (0 - 255)

#endif // PINOUT_CONFIG_H
