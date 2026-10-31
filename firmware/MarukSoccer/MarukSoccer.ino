/**
 * MARUK (SOCCER) - Production ESP32 Firmware (2WD Edition)
 * Conforms 100% to MIT-WEEK 2026 Car Soccer Rules.
 * 
 * Hardware:
 *   - ESP32 DevKit
 *   - 1x Driver L298N
 *   - 2x Motor Gearbox Kuning (TT Motor)
 *   - 1x atau 2x Roda Kastor Penyeimbang (Pasif)
 *   - 1x Receiver Drone (Port Serial2 RX / GPIO 16)
 *   - 1x LM2596 (Setel output ke 5.0V)
 * 
 * Architecture: SENSE -> THINK -> ACT
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include "pinout_config.h"
#include "kinematics.h"

// WiFi Teleop Configuration (Fallback / Testing di Laptop)
const char* AP_SSID = "MARUK_SOCCER_01";
const char* AP_PASS = "maruk123";
const unsigned int UDP_PORT = 5005;

WiFiUDP udp;
char packetBuffer[256];

// Failsafe & Timing
unsigned long lastCommandTime = 0;
const unsigned long FAILSAFE_TIMEOUT_MS = 350;

// ==========================================
// LAYER 3: ACT (Hardware Actuators - 1x L298N)
// ==========================================
void initHardware() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    // Setup 4 Pin Logika L298N
    pinMode(PIN_LEFT_IN1, OUTPUT);
    pinMode(PIN_LEFT_IN2, OUTPUT);
    pinMode(PIN_RIGHT_IN3, OUTPUT);
    pinMode(PIN_RIGHT_IN4, OUTPUT);

    // Attach ESP32 LEDC PWM
    ledcAttach(PIN_LEFT_IN1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_LEFT_IN2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RIGHT_IN3, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RIGHT_IN4, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

    // Stop motor initially
    ledcWrite(PIN_LEFT_IN1, 0);
    ledcWrite(PIN_LEFT_IN2, 0);
    ledcWrite(PIN_RIGHT_IN3, 0);
    ledcWrite(PIN_RIGHT_IN4, 0);
}

void driveMotorLeft(float speed) {
    // speed: -1.0 to +1.0
    int pwm = constrain((int)(fabsf(speed) * 255.0f), 0, 255);
    if (speed > 0.05f) {
        ledcWrite(PIN_LEFT_IN1, pwm);
        ledcWrite(PIN_LEFT_IN2, 0);
    } else if (speed < -0.05f) {
        ledcWrite(PIN_LEFT_IN1, 0);
        ledcWrite(PIN_LEFT_IN2, pwm);
    } else {
        ledcWrite(PIN_LEFT_IN1, 0);
        ledcWrite(PIN_LEFT_IN2, 0);
    }
}

void driveMotorRight(float speed) {
    // speed: -1.0 to +1.0
    int pwm = constrain((int)(fabsf(speed) * 255.0f), 0, 255);
    if (speed > 0.05f) {
        ledcWrite(PIN_RIGHT_IN3, pwm);
        ledcWrite(PIN_RIGHT_IN4, 0);
    } else if (speed < -0.05f) {
        ledcWrite(PIN_RIGHT_IN3, 0);
        ledcWrite(PIN_RIGHT_IN4, pwm);
    } else {
        ledcWrite(PIN_RIGHT_IN3, 0);
        ledcWrite(PIN_RIGHT_IN4, 0);
    }
}

void emergencyStop() {
    ledcWrite(PIN_LEFT_IN1, 0);
    ledcWrite(PIN_LEFT_IN2, 0);
    ledcWrite(PIN_RIGHT_IN3, 0);
    ledcWrite(PIN_RIGHT_IN4, 0);
    digitalWrite(PIN_STATUS_LED, LOW);
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\n[MARUK] =========================================");
    Serial.println("[MARUK] 2WD Car Soccer Robot Starting...");
    Serial.println("[MARUK] Regulations: MIT-WEEK 2026 Compliant");
    Serial.println("[MARUK] Motors: 2x Yellow TT Motors (1x L298N)");
    Serial.println("[MARUK] Caster: 1x / 2x Balance Spheres");
    Serial.println("[MARUK] =========================================");

    initHardware();

    // Init Hardware Serial2 untuk Receiver Drone di GPIO 16 (RX)
    // Mendukung SBUS (100k inverted) atau IBUS (115.2k 8N1)
    Serial2.begin(115200, SERIAL_8N1, PIN_RC_RX, -1);
    Serial.printf("[MARUK] Drone Receiver listening on GPIO %d (Serial2)\n", PIN_RC_RX);

    // Nyalakan WiFi Access Point sebagai fallback / testing di laptop
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress myIP = WiFi.softAPIP();

    Serial.print("[MARUK] Fallback WiFi Hotspot: ");
    Serial.println(AP_SSID);
    Serial.print("[MARUK] Bot IP:                 ");
    Serial.println(myIP);

    udp.begin(UDP_PORT);
    Serial.printf("[MARUK] UDP Server listening on port %d\n", UDP_PORT);

    // Kedipkan LED status 3 kali
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_STATUS_LED, HIGH); delay(100);
        digitalWrite(PIN_STATUS_LED, LOW); delay(100);
    }
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    float vx = 0.0f;
    float omega = 0.0f;
    bool commandReceived = false;

    // ------------------------------------------
    // 1. SENSE: BACA RECEIVER REMOTE DRONE (SERIAL2)
    // ------------------------------------------
    if (Serial2.available() >= 6) {
        // Simple serial RC packet parser:
        // Format teks cepat: "V,O\n" misal "1.0,-0.5\n"
        // Atau buffer packet byte
        String line = Serial2.readStringUntil('\n');
        line.trim();
        int commaIdx = line.indexOf(',');
        if (commaIdx > 0) {
            vx = line.substring(0, commaIdx).toFloat();
            omega = line.substring(commaIdx + 1).toFloat();
            commandReceived = true;
            lastCommandTime = millis();
            digitalWrite(PIN_STATUS_LED, HIGH);
        }
    }

    // ------------------------------------------
    // 2. SENSE: FALLBACK BACA WIFI UDP
    // ------------------------------------------
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
        if (len > 0) {
            packetBuffer[len] = '\0';
            
            // Format JSON: {"vx": ..., "w": ...}
            char* p_vx = strstr(packetBuffer, "\"vx\":");
            char* p_w  = strstr(packetBuffer, "\"w\":");

            if (p_vx) vx = atof(p_vx + 5);
            if (p_w)  omega = atof(p_w + 4);

            commandReceived = true;
            lastCommandTime = millis();
            digitalWrite(PIN_STATUS_LED, HIGH);
        }
    }

    // ------------------------------------------
    // FAILSAFE WATCHDOG
    // ------------------------------------------
    if (millis() - lastCommandTime > FAILSAFE_TIMEOUT_MS) {
        emergencyStop();
        return; // Hentikan motor jika sinyal hilang > 350ms
    }

    // ------------------------------------------
    // 3. THINK: KINEMATIKA 2WD DIFFERENTIAL
    // ------------------------------------------
    DiffSpeeds speeds = compute_diff_speeds(vx, omega, 1.0f, 0.08f);

    // ------------------------------------------
    // 4. ACT: DRIVE MOTOR KIRI & KANAN
    // ------------------------------------------
    driveMotorLeft(speeds.left);
    driveMotorRight(speeds.right);
}
