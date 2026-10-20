/**
 * MARUK (SOCCER) - Production ESP32 Firmware
 * Micro-Architecture for Robotic Universal Kernel (Soccer Edition)
 * 
 * Hardware: ESP32 + 4-Wheel Mecanum Drive + Dribbler + Kicker
 * Protocol: UDP Teleoperation over WiFi (Port 5005)
 * Architecture: SENSE -> THINK -> ACT
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include "pinout_config.h"
#include "kinematics.h"

// WiFi Configuration
// Mode AP: ESP32 pembuat hotspot sendiri, langsung konek laptop ke sini!
const char* AP_SSID = "MARUK_SOCCER_01";
const char* AP_PASS = "maruk123";
const unsigned int UDP_PORT = 5005;

WiFiUDP udp;
char packetBuffer[256];

// State & Failsafe
unsigned long lastPacketTime = 0;
const unsigned long FAILSAFE_TIMEOUT_MS = 350;

// Kicker Pulse Safety Timer
unsigned long kickerTriggerTime = 0;
const unsigned long KICKER_PULSE_MS = 50; // 50ms pulse max
bool isKicking = false;

// ==========================================
// LAYER 3: ACT (Hardware Motor Actuators)
// ==========================================
void initHardware() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    // Kicker Pin
    pinMode(PIN_KICKER_SOLENOID, OUTPUT);
    digitalWrite(PIN_KICKER_SOLENOID, LOW);

    // Motor Direction Pins
    pinMode(PIN_FL_DIR, OUTPUT);
    pinMode(PIN_FR_DIR, OUTPUT);
    pinMode(PIN_RL_DIR, OUTPUT);
    pinMode(PIN_RR_DIR, OUTPUT);
    pinMode(PIN_DRIBBLER_DIR, OUTPUT);

    // Setup ESP32 PWM Channels (LEDC)
    // fl=0, fr=1, rl=2, rr=3, dribbler=4
    ledcAttach(PIN_FL_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_FR_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RL_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RR_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_DRIBBLER_PWM, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

    // Stop all motors initially
    ledcWrite(PIN_FL_PWM, 0);
    ledcWrite(PIN_FR_PWM, 0);
    ledcWrite(PIN_RL_PWM, 0);
    ledcWrite(PIN_RR_PWM, 0);
    ledcWrite(PIN_DRIBBLER_PWM, 0);
}

void driveSingleMotor(int pwmPin, int dirPin, float speedNormalized) {
    // speedNormalized: -1.0 to +1.0
    int pwmVal = constrain((int)(fabsf(speedNormalized) * 255.0f), 0, 255);
    bool forward = (speedNormalized >= 0.0f);

    digitalWrite(dirPin, forward ? HIGH : LOW);
    ledcWrite(pwmPin, pwmVal);
}

void actuateMotors(const WheelSpeeds& speeds, bool dribbleActive, bool kickTrigger) {
    // 1. Wheels
    driveSingleMotor(PIN_FL_PWM, PIN_FL_DIR, speeds.fl);
    driveSingleMotor(PIN_FR_PWM, PIN_FR_DIR, speeds.fr);
    driveSingleMotor(PIN_RL_PWM, PIN_RL_DIR, speeds.rl);
    driveSingleMotor(PIN_RR_PWM, PIN_RR_DIR, speeds.rr);

    // 2. Dribbler Roller
    digitalWrite(PIN_DRIBBLER_DIR, HIGH);
    ledcWrite(PIN_DRIBBLER_PWM, dribbleActive ? 220 : 0);

    // 3. Kicker Solenoid (Pulse safety)
    if (kickTrigger && !isKicking) {
        digitalWrite(PIN_KICKER_SOLENOID, HIGH);
        kickerTriggerTime = millis();
        isKicking = true;
    }

    if (isKicking && (millis() - kickerTriggerTime >= KICKER_PULSE_MS)) {
        digitalWrite(PIN_KICKER_SOLENOID, LOW);
        isKicking = false;
    }
}

void emergencyStop() {
    ledcWrite(PIN_FL_PWM, 0);
    ledcWrite(PIN_FR_PWM, 0);
    ledcWrite(PIN_RL_PWM, 0);
    ledcWrite(PIN_RR_PWM, 0);
    ledcWrite(PIN_DRIBBLER_PWM, 0);
    digitalWrite(PIN_KICKER_SOLENOID, LOW);
    isKicking = false;
    digitalWrite(PIN_STATUS_LED, LOW);
}

// ==========================================
// SETUP & MAIN LOOP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\n[MARUK] =========================================");
    Serial.println("[MARUK] ESP32 Soccer Bot Firmware Starting...");
    Serial.println("[MARUK] =========================================");

    initHardware();

    // Start WiFi Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress myIP = WiFi.softAPIP();

    Serial.print("[MARUK] Hotspot AP Created: ");
    Serial.println(AP_SSID);
    Serial.print("[MARUK] Bot IP Address:     ");
    Serial.println(myIP);

    udp.begin(UDP_PORT);
    Serial.printf("[MARUK] UDP Server listening on port %d\n", UDP_PORT);

    // Blink status LED 3 times
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_STATUS_LED, HIGH); delay(100);
        digitalWrite(PIN_STATUS_LED, LOW); delay(100);
    }
}

void loop() {
    // ------------------------------------------
    // LAYER 1: SENSE (Read UDP Packet)
    // ------------------------------------------
    float vx = 0.0f, vy = 0.0f, omega = 0.0f;
    int kick = 0, dribble = 0;
    bool packetReceived = false;

    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
        if (len > 0) {
            packetBuffer[len] = '\0';
            
            // Format: {"vx": ..., "vy": ..., "w": ..., "k": ..., "d": ...}
            // Fast zero-dependency parsing:
            char* p_vx = strstr(packetBuffer, "\"vx\":");
            char* p_vy = strstr(packetBuffer, "\"vy\":");
            char* p_w  = strstr(packetBuffer, "\"w\":");
            char* p_k  = strstr(packetBuffer, "\"k\":");
            char* p_d  = strstr(packetBuffer, "\"d\":");

            if (p_vx) vx = atof(p_vx + 5);
            if (p_vy) vy = atof(p_vy + 5);
            if (p_w)  omega = atof(p_w + 4);
            if (p_k)  kick = atoi(p_k + 4);
            if (p_d)  dribble = atoi(p_d + 4);

            lastPacketTime = millis();
            packetReceived = true;
            digitalWrite(PIN_STATUS_LED, HIGH);
        }
    }

    // ------------------------------------------
    // SAFETY FAILSAFE WATCHDOG
    // ------------------------------------------
    if (millis() - lastPacketTime > FAILSAFE_TIMEOUT_MS) {
        emergencyStop();
        return; // Skip execution if connection lost
    }

    // ------------------------------------------
    // LAYER 2: THINK (Kinematics)
    // ------------------------------------------
    WheelSpeeds targetSpeeds = compute_mecanum_speeds(vx, vy, omega, 1.0f, 0.08f);

    // ------------------------------------------
    // LAYER 3: ACT (Drive Hardware)
    // ------------------------------------------
    actuateMotors(targetSpeeds, dribble > 0, kick > 0);
}
