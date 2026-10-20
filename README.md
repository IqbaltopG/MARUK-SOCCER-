# MARUK (SOCCER) - RC Soccer Bot Platform

> **Micro-Architecture for Robotic Universal Kernel (Soccer Edition)**  
> 4-Wheel Mecanum Drive | UDP Teleoperation | Webots 3D Simulation | ESP32 Firmware

---

## 📁 Struktur Direktori

```
MARUK(SOCCER)/
├── common/                      # Shared Kinematics Math (Pure Python)
│   ├── kinematics.py            # Rumus 4-wheel mecanum mixer
│   └── test_kinematics.py       # Unit test otomatis
│
├── teleop/                      # Unified Remote Control Station
│   └── udp_sender.py            # Transmitter (Keyboard WASD + USB Gamepad)
│
├── worlds/                      # Simulasi 3D Webots
│   └── soccer_field.wbt         # Lapangan bola, bola fisik, gawang, robot MARUK
│
├── controllers/                 # Otak Robot di Simulasi
│   └── maruk_soccer/
│       └── maruk_soccer.py      # Controller Webots (Sense UDP -> Think -> Act)
│
└── firmware/                    # Hardware Fisik (ESP32 C++ Arduino)
    ├── MarukSoccer.ino          # Firmware utama (WiFi AP + UDP Receiver + Failsafe)
    ├── kinematics.h             # Rumus mecanum versi C++
    └── pinout_config.h          # Konfigurasi pin motor & kicker
```

---

## 🎮 Cara Mengemudikan Robot di Webots (Quickstart)

### Langkah 1: Buka Lapangan Bola di Webots
Buka terminal dan jalankan:
```bash
webots "/home/ambatron/MARUK(SOCCER)/worlds/soccer_field.wbt"
```
Tekan tombol **Play (▶️)** di Webots.

### Langkah 2: Buka Stasiun Remote Control (Teleop)
Buka tab terminal baru, jalankan transmitter:
```bash
python3 "/home/ambatron/MARUK(SOCCER)/teleop/udp_sender.py"
```
Jendela status teleop akan muncul! Klik pada jendela tersebut untuk mengendalikan robot:

| Tombol Keyboard | Fungsi |
|---|---|
| `W` / `S` | Maju / Mundur ($V_x$) |
| `A` / `D` | **Strafe Kiri / Kanan** ($V_y$) tanpa muter badan |
| `Q` / `E` | Putar Balik CCW / CW ($\omega$) |
| `SPACE` | **Tendang Bola** (Kicker) |
| `LSHIFT` / `R` | Nyalakan / Matikan Dribbler Roller |
| `ESC` | Keluar |

> *Tips*: Jika punya stik USB (PS4/Xbox/Logitech), colok saja ke laptop! Script `udp_sender.py` otomatis mendeteksi analog stick dan tombolnya.

---

## 🏎️ Cara Flashing ke Hardware Fisik (ESP32)

1. Buka Arduino IDE atau VSCode PlatformIO.
2. Buka file `/home/ambatron/MARUK(SOCCER)/firmware/MarukSoccer.ino`.
3. Pasang board: **ESP32 Dev Module**.
4. Upload firmware ke ESP32.
5. ESP32 akan membuat hotspot WiFi sendiri:
   - **SSID**: `MARUK_SOCCER_01`
   - **Password**: `maruk123`
6. Sambungkan WiFi laptopmu ke hotspot `MARUK_SOCCER_01`.
7. Jalankan teleop ke IP ESP32:
```bash
python3 "/home/ambatron/MARUK(SOCCER)/teleop/udp_sender.py" --ip 192.168.4.1
```
Robot fisikmu siap tanding di lapangan kampus! 🏆

---

## 🧠 Rumus Kinematika 4-Roda Mecanum (The Think Layer)

$$V_{FL} = V_x + V_y + \omega$$
$$V_{FR} = V_x - V_y - \omega$$
$$V_{RL} = V_x - V_y + \omega$$
$$V_{RR} = V_x + V_y - \omega$$

Semua perhitungan dinormalisasi otomatis agar jika ada roda yang melebihi batas 100%, rasio pergerakan tetap presisi tanpa melenceng.
