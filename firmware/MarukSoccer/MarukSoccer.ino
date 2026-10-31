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

// ==========================================
// 🛠️ VARIABEL TUNING FISIK (DUNIA NYATA)
// Ubah angka-angka di bawah ini saat setting di lapangan:
// ==========================================

// 1. Kecepatan Maksimal (Batas Gas: 0 - 255)
//    Setel 200-220 jika di karpet licin / bola mudah mental
//    Setel 255 untuk kecepatan sprint penuh
int MAX_PWM = 255;

// 2. Ambang Awal Gerak Motor (Deadband: 40 - 80)
//    Motor kuning butuh voltase minimum untuk mulai muter gearbox.
//    Jika stik digerakkan sedikit tapi motor hanya berdengung belum jalan, naikkan angka ini (misal 65-70).
int MIN_PWM = 60;

// 3. Kalibrasi Jalan Lurus (Trim Motor Kiri vs Kanan: 0.80 - 1.00)
//    Dua motor kuning murah sering memiliki selisih RPM pabrik.
//    - Jika robot belok sendiri ke KIRI saat maju lurus -> kurangi TRIM_RIGHT (misal 0.93)
//    - Jika robot belok sendiri ke KANAN saat maju lurus -> kurangi TRIM_LEFT (misal 0.93)
float TRIM_LEFT  = 1.00f;
float TRIM_RIGHT = 1.00f;

// 4. Batas Waktu Darurat Failsafe (Milidetik)
//    Jika sinyal remote terputus lebih dari waktu ini, motor otomatis mati.
const unsigned long FAILSAFE_TIMEOUT_MS = 350;

// ==========================================
// WIFI TELEOP CONFIGURATION (FALLBACK / TESTING)
// ==========================================
const char* AP_SSID = "MARUK_SOCCER_01";
const char* AP_PASS = "maruk123";
const unsigned int UDP_PORT = 5005;

WiFiUDP udp;
char packetBuffer[256];
unsigned long lastCommandTime = 0;

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
    if (fabsf(speed) < 0.05f) {
        ledcWrite(PIN_LEFT_IN1, 0);
        ledcWrite(PIN_LEFT_IN2, 0);
        return;
    }
    float scaled = fabsf(speed) * TRIM_LEFT;
    int pwm = MIN_PWM + (int)(scaled * (MAX_PWM - MIN_PWM));
    pwm = constrain(pwm, 0, 255);

    if (speed > 0.0f) {
        ledcWrite(PIN_LEFT_IN1, pwm);
        ledcWrite(PIN_LEFT_IN2, 0);
    } else {
        ledcWrite(PIN_LEFT_IN1, 0);
        ledcWrite(PIN_LEFT_IN2, pwm);
    }
}

void driveMotorRight(float speed) {
    if (fabsf(speed) < 0.05f) {
        ledcWrite(PIN_RIGHT_IN3, 0);
        ledcWrite(PIN_RIGHT_IN4, 0);
        return;
    }
    float scaled = fabsf(speed) * TRIM_RIGHT;
    int pwm = MIN_PWM + (int)(scaled * (MAX_PWM - MIN_PWM));
    pwm = constrain(pwm, 0, 255);

    if (speed > 0.0f) {
        ledcWrite(PIN_RIGHT_IN3, pwm);
        ledcWrite(PIN_RIGHT_IN4, 0);
    } else {
        ledcWrite(PIN_RIGHT_IN3, 0);
        ledcWrite(PIN_RIGHT_IN4, pwm);
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
    Serial.printf("[MARUK] Config: MIN_PWM=%d | MAX_PWM=%d\n", MIN_PWM, MAX_PWM);
    Serial.printf("[MARUK] Trim:   LEFT=%.2f | RIGHT=%.2f\n", TRIM_LEFT, TRIM_RIGHT);
    Serial.println("[MARUK] =========================================");

    initHardware();

    // Init Hardware Serial2 untuk Receiver Drone di GPIO 16 (RX)
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
    if (Serial2.available() >= 4) {
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
        return;
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
