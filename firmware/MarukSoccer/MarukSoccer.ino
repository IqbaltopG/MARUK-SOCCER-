/**
 * MARUK (SOCCER) - Production ESP32 Firmware (2WD Edition)
 * Conforms 100% to MIT-WEEK 2026 Car Soccer Rules.
 * 
 * Fitur Tambahan On-The-Fly Tuning (Tanpa Perlu Flash Ulang):
 *   1. Web Dashboard di Browser HP (Buka http://192.168.4.1)
 *   2. Simpan permanen ke memory chip NVS (Preferences)
 *   3. Serial Receiver Remote Drone (GPIO 16) + Remote Knob Tuning
 *   4. WiFi UDP Teleop Fallback (Port 5005)
 *   5. Auto-Failsafe Watchdog 350ms
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include "pinout_config.h"
#include "kinematics.h"

// ==========================================
// 🛠️ VARIABEL TUNING FISIK
// Nilai default ini otomatis ditimpa oleh memori NVS / Web HP
// ==========================================
int MAX_PWM = 255;          // Batas gas atas (0 - 255)
int MIN_PWM = 60;           // Ambang awal gerak motor (40 - 80)
float TRIM_LEFT  = 1.00f;   // Kalibrasi lari lurus motor kiri
float TRIM_RIGHT = 1.00f;   // Kalibrasi lari lurus motor kanan
const unsigned long FAILSAFE_TIMEOUT_MS = 350;

// WiFi Access Point & Web Server di HP
const char* AP_SSID = "MARUK_SOCCER_01";
const char* AP_PASS = "maruk123";
const unsigned int UDP_PORT = 5005;

WiFiUDP udp;
WebServer server(80);
Preferences prefs;
char packetBuffer[256];
unsigned long lastCommandTime = 0;

// ==========================================
// 📱 WEB TUNING PORTAL (BUKA DI HP: 192.168.4.1)
// ==========================================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MARUK Soccer Tuning</title>
  <style>
    body { font-family: monospace; background: #14181e; color: #fff; padding: 20px; }
    h2 { color: #00ffb4; text-align: center; }
    .card { background: #1c222b; border: 1px solid #2f3846; border-radius: 8px; padding: 16px; margin-bottom: 16px; }
    label { display: block; margin-top: 12px; color: #a0aec0; }
    input[type=range] { width: 100%; margin: 8px 0; }
    .val { float: right; color: #00ffb4; font-weight: bold; }
    button { width: 100%; background: #00ffb4; color: #14181e; border: none; padding: 12px; font-size: 16px; font-weight: bold; border-radius: 6px; cursor: pointer; margin-top: 16px; }
    button:active { background: #00cc90; }
    .status { text-align: center; color: #718096; font-size: 12px; margin-top: 10px; }
  </style>
</head>
<body>
  <h2>MARUK 2WD TUNER</h2>
  <div class="card">
    <label>MAX SPEED (Batas Gas): <span class="val" id="v_max">255</span></label>
    <input type="range" id="max_pwm" min="120" max="255" value="255" oninput="document.getElementById('v_max').innerText=this.value">

    <label>MIN PWM (Start Motor): <span class="val" id="v_min">60</span></label>
    <input type="range" id="min_pwm" min="40" max="100" value="60" oninput="document.getElementById('v_min').innerText=this.value">

    <label>TRIM KIRI (Lurus): <span class="val" id="v_tl">1.00</span></label>
    <input type="range" id="trim_l" min="70" max="100" value="100" oninput="document.getElementById('v_tl').innerText=(this.value/100).toFixed(2)">

    <label>TRIM KANAN (Lurus): <span class="val" id="v_tr">1.00</span></label>
    <input type="range" id="trim_r" min="70" max="100" value="100" oninput="document.getElementById('v_tr').innerText=(this.value/100).toFixed(2)">

    <button onclick="saveConfig()">SIMPAN SETTINGAN KE CHIP</button>
  </div>
  <div class="status" id="stat">Buka di HP saat konek WiFi MARUK_SOCCER_01</div>

  <script>
    function saveConfig() {
      let maxP = document.getElementById('max_pwm').value;
      let minP = document.getElementById('min_pwm').value;
      let tl = (document.getElementById('trim_l').value / 100).toFixed(2);
      let tr = (document.getElementById('trim_r').value / 100).toFixed(2);
      
      document.getElementById('stat').innerText = "Menyimpan ke chip...";
      fetch(`/save?max=${maxP}&min=${minP}&tl=${tl}&tr=${tr}`)
        .then(r => r.text())
        .then(d => {
          document.getElementById('stat').innerText = "Tersimpan! Efek langsung aktif.";
        })
        .catch(e => {
          document.getElementById('stat').innerText = "Gagal kirim settingan.";
        });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", HTML_PAGE);
}

void handleSave() {
    if (server.hasArg("max")) MAX_PWM = server.arg("max").toInt();
    if (server.hasArg("min")) MIN_PWM = server.arg("min").toInt();
    if (server.hasArg("tl"))  TRIM_LEFT = server.arg("tl").toFloat();
    if (server.hasArg("tr"))  TRIM_RIGHT = server.arg("tr").toFloat();

    // Simpan ke NVS flash memory permanen
    prefs.begin("maruk", false);
    prefs.putInt("max_pwm", MAX_PWM);
    prefs.putInt("min_pwm", MIN_PWM);
    prefs.putFloat("trim_l", TRIM_LEFT);
    prefs.putFloat("trim_r", TRIM_RIGHT);
    prefs.end();

    Serial.printf("[MARUK NVS] Saved: MAX=%d | MIN=%d | TL=%.2f | TR=%.2f\n", MAX_PWM, MIN_PWM, TRIM_LEFT, TRIM_RIGHT);
    server.send(200, "text/plain", "OK");
}

// ==========================================
// LAYER 3: ACT (Hardware Actuators - 1x L298N)
// ==========================================
void initHardware() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    pinMode(PIN_LEFT_IN1, OUTPUT);
    pinMode(PIN_LEFT_IN2, OUTPUT);
    pinMode(PIN_RIGHT_IN3, OUTPUT);
    pinMode(PIN_RIGHT_IN4, OUTPUT);

    ledcAttach(PIN_LEFT_IN1, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_LEFT_IN2, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RIGHT_IN3, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttach(PIN_RIGHT_IN4, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

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

    // 1. Muat settingan tersimpan dari flash NVS (jika pernah disimpan lewat web HP)
    prefs.begin("maruk", true);
    MAX_PWM    = prefs.getInt("max_pwm", MAX_PWM);
    MIN_PWM    = prefs.getInt("min_pwm", MIN_PWM);
    TRIM_LEFT  = prefs.getFloat("trim_l", TRIM_LEFT);
    TRIM_RIGHT = prefs.getFloat("trim_r", TRIM_RIGHT);
    prefs.end();

    Serial.println("\n[MARUK] =========================================");
    Serial.println("[MARUK] 2WD Car Soccer Robot Starting...");
    Serial.println("[MARUK] Regulations: MIT-WEEK 2026 Compliant");
    Serial.printf("[MARUK] Config: MIN_PWM=%d | MAX_PWM=%d\n", MIN_PWM, MAX_PWM);
    Serial.printf("[MARUK] Trim:   LEFT=%.2f | RIGHT=%.2f\n", TRIM_LEFT, TRIM_RIGHT);
    Serial.println("[MARUK] =========================================");

    initHardware();

    // 2. Hardware Serial2 untuk Receiver Remote Drone (GPIO 16)
    Serial2.begin(115200, SERIAL_8N1, PIN_RC_RX, -1);
    Serial.printf("[MARUK] Drone Receiver listening on GPIO %d (Serial2)\n", PIN_RC_RX);

    // 3. Hotspot WiFi & Web Server
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress myIP = WiFi.softAPIP();

    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();

    Serial.print("[MARUK] Web Tuner IP: http://");
    Serial.println(myIP);

    udp.begin(UDP_PORT);
    Serial.printf("[MARUK] UDP Teleop Port: %d\n", UDP_PORT);

    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_STATUS_LED, HIGH); delay(100);
        digitalWrite(PIN_STATUS_LED, LOW); delay(100);
    }
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    // Layani request web browser HP jika sedang setting slider
    server.handleClient();

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
            
            // Cek apakah ada channel ke-3 untuk knob gas
            int secondComma = line.indexOf(',', commaIdx + 1);
            if (secondComma > 0) {
                omega = line.substring(commaIdx + 1, secondComma).toFloat();
                float knobSpeed = line.substring(secondComma + 1).toFloat(); // 0.0 - 1.0 dari knob remote
                if (knobSpeed > 0.1f) {
                    MAX_PWM = (int)(knobSpeed * 255.0f); // Fast tune langsung dari knob remote!
                }
            } else {
                omega = line.substring(commaIdx + 1).toFloat();
            }

            commandReceived = true;
            lastCommandTime = millis();
            digitalWrite(PIN_STATUS_LED, HIGH);
        }
    }

    // ------------------------------------------
    // 2. SENSE: FALLBACK BACA WIFI UDP TELEOP
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
    // 3. THINK: KINEMATIKA 2WD
    // ------------------------------------------
    DiffSpeeds speeds = compute_diff_speeds(vx, omega, 1.0f, 0.08f);

    // ------------------------------------------
    // 4. ACT: DRIVE MOTOR KIRI & KANAN
    // ------------------------------------------
    driveMotorLeft(speeds.left);
    driveMotorRight(speeds.right);
}
