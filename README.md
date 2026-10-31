# MARUK (SOCCER) - 2WD Car Soccer Robot

> **Micro-Architecture for Robotic Universal Kernel (Soccer Edition)**  
> **100% Compliant dengan Regulasi Resmi MIT-WEEK 2026 (Fakultas Teknik Univ. Mulawarman)**  
> 2x Motor Gearbox Kuning | 1x Driver L298N | Roda Kastor | Bumper Papan Lurus | Remote Drone + Teleop

---

## 📋 Spesifikasi Robot & Regulasi Lomba

Robot ini dirancang khusus untuk mematuhi **Buku Panduan Teknis (Juknis) Resmi MIT-WEEK 2026**:

| Parameter | Regulasi Resmi | Desain MARUK 2WD | Status |
|---|---|---|---|
| **Dimensi Maksimal** | $18 \times 18$ cm, tinggi maks 15 cm | **$16 \times 15 \times 7$ cm** |  Lolos Mutlak |
| **Bobot Maksimal** | Maksimal 2.0 kg | **~600 gram** |  Sangat Enteng |
| **Bumper Depan** | **HANYA PAPAN LURUS DATAR**, tanpa lekukan/capit/bibir | **Plat Persegi Panjang Rata ($16 \times 4$ cm)** |  100% Patuh |
| **Jenis Motor DC** | **Wajib Gearbox Kuning Standar (TT Motor)** | **2x Motor Kuning Standar (Rasio 1:48)** |  Resmi |
| **Driver Motor** | **Wajib L298N (Maksimal 2 buah)** | **1x Modul L298N Dual H-Bridge** |  Resmi & Hemat |
| **Penendang / Dribbler** | **DILARANG KERAS (No Kicker, No Roller)** | **Tanpa Penendang / Dribbler Aktif** |  Aman dari Sanksi |
| **Baterai** | Maksimal 12 Volt | Baterai LiPo 2S (7.4V) / 3S (11.1V) / Aki |  Aman |
| **Kendali** | Wireless Smartphone / Joystick | **Remote Drone (Serial RX) + WiFi UDP Fallback** |  Resmi |
| **Arena Pertandingan** | $244 \times 122$ cm, Gawang $38 \times 20$ cm | Dimodelkan identik di Webots |  Akurat |
| **Bola** | Bola pingpong mainan $\varnothing$ 4 cm | Dimodelkan 4 cm, 8 gram di Webots |  Akurat |

---

## 🛠️ Panduan Tuning di Dunia Nyata (Hardware Calibration & Variables)

Di dunia nyata, motor kuning dan karpet lapangan asli memiliki karakteristik fisika berbeda dari simulasi. Buka file [`firmware/MarukSoccer/MarukSoccer.ino`](firmware/MarukSoccer/MarukSoccer.ino) baris 26-45, ada 4 variabel utama yang bisa kamu ubah dengan sangat mudah:

### 1. `MAX_PWM` (Batas Gas Tertinggi: 0 - 255)
```cpp
int MAX_PWM = 255;  // Default: 255 (Full Gas 100%)
```
* **Kapan diubah?** Jika robot terasa terlalu liar, roda selip di karpet licin, atau bola pingpong mudah membal kejauhan saat ditabrak:
  * Turunkan menjadi `200` atau `220` untuk kontrol bola yang lebih santai dan presisi.
  * Tetap gunakan `255` jika ingin kecepatan sprint penuh.

### 2. `MIN_PWM` (Ambang Awal Putaran Motor: 40 - 80)
```cpp
int MIN_PWM = 60;   // Default: 60
```
* **Kapan diubah?** Motor kuning murah membutuhkan voltase minimum agar gearbox plastik mulai bergerak:
  * **Gejala**: Saat stik remote disentuh sedikit, motor hanya berdengung (*buzzing*) tapi roda belum mau berputar.
  * **Solusi**: Naikkan nilainya ke `65` atau `75` sampai roda langsung responsif begitu stik digerakkan sedikit.

### 3. `TRIM_LEFT` dan `TRIM_RIGHT` (Kalibrasi Lari Lurus: 0.80 - 1.00)
```cpp
float TRIM_LEFT  = 1.00f;
float TRIM_RIGHT = 1.00f;
```
* **Kapan diubah?** Dua motor DC kuning murah hampir tidak pernah memiliki RPM yang 100% sama persis dari pabrik. Akibatnya saat kamu dorong stik maju lurus, robot sering berbelok tipis ke kiri atau kanan dengan sendirinya:
  * **Jika robot belok sendiri ke KIRI**: Artinya motor kanan lebih kencang. Turunkan `TRIM_RIGHT` menjadi `0.93` atau `0.90`.
  * **Jika robot belok sendiri ke KANAN**: Artinya motor kiri lebih kencang. Turunkan `TRIM_LEFT` menjadi `0.93` atau `0.90`.

### 4. `FAILSAFE_TIMEOUT_MS` (Proteksi Sinyal Darurat)
```cpp
const unsigned long FAILSAFE_TIMEOUT_MS = 350; // 350 milidetik
```
* Jika remote drone mati atau sinyal terputus lebih dari 0.35 detik, motor otomatis mati seketika agar robot tidak kabur menabrak arena.

---

## 🛞 Pilihan Roda Penyeimbang: 1 Kastor vs 2 Kastor

Robot ini menggunakan sistem **2WD Differential Drive** (2 roda penggerak utama di belakang). Untuk penyeimbang depan, kamu bisa memilih:

* **Opsi 1: Satu Roda Kastor (Tricycle Drive - Depan Tengah)**
  * *Kelebihan*: Hambatan gesek lantai paling kecil, putaran balik di tempat (*zero-turn spin*) super licin dan cepat.
  * *Kekurangan*: Jika belok terlalu mendadak dengan kecepatan tinggi dan baterai berat di belakang, bodi bisa sedikit miring ke samping.
* **Opsi 2: Dua Roda Kastor (Quad-Support Drive - Depan Kiri & Kanan)**
  * *Kelebihan*: Bodi sangat stabil, papan bumper depan selalu rata sejajar karpet, sangat kokoh saat adu dorong (*tackle*) dengan robot lawan.
  * *Kekurangan*: Hambatan gesek lantai bertambah sedikit.

> **Catatan Software**: Secara kode program dan rumus kinematika, **1 kastor maupun 2 kastor kodenya sama 100%**, karena roda kastor adalah roda pasif (berputar bebas 360° tanpa motor).

---

## 🔌 Tabel Pinout Perkabelan Lengkap (Wiring Cheat Sheet)

Semua colokan kabel dirancang hemat pin menggunakan **1 modul driver L298N** dan **1 board ESP32**:

### 1. Jalur Daya & Catu Daya (LM2596 & Baterai)
> ⚠️ **PENTING**: Putar trimpot (baut kuningan) pada LM2596 menggunakan obeng minus sambil diukur multimeter hingga voltase output tepat **5.0 Volt** sebelum dicolok ke ESP32!

| Dari | Ke | Keterangan |
|---|---|---|
| **Baterai (+) (7.4V - 12V)** | LM2596 `IN+` **DAN** L298N `12V / VCC` | Pakai kabel tebal tembaga |
| **Baterai (-) (GND)** | LM2596 `IN-` **DAN** L298N `GND` | Common Ground |
| **LM2596 `OUT+` (Setel 5.0V)** | Pin **`VIN` (atau `5V`)** ESP32 | Daya aman ESP32 |
| **LM2596 `OUT-`** | Pin **`GND`** ESP32 | Ground bersama |

### 2. Koneksi ESP32 ke Driver L298N
*(Pasang tutup jumper bawaan pada pin `ENA` dan `ENB` L298N agar selalu aktif)*:

| Pin ESP32 | Pin Driver L298N | Output Motor | Fungsi Gerakan |
|---|---|---|---|
| **GPIO 13** | `IN1` | `OUT1` (Motor Kiri) | Motor Kiri Maju (PWM) |
| **GPIO 12** | `IN2` | `OUT2` (Motor Kiri) | Motor Kiri Mundur (PWM) |
| **GPIO 14** | `IN3` | `OUT3` (Motor Kanan)| Motor Kanan Maju (PWM) |
| **GPIO 27** | `IN4` | `OUT4` (Motor Kanan)| Motor Kanan Mundur (PWM) |
| **GND** | `GND` | - | Ground logika wajib terhubung |

### 3. Koneksi ESP32 ke Receiver Remote Drone
| Pin Receiver Drone | Pin ESP32 | Keterangan |
|---|---|---|
| **Signal (Serial SBUS / IBUS)** | **GPIO 16 (RX2)** | Cukup 1 kabel data baca semua channel |
| **VCC (+)** | **5V** (dari LM2596 / pin 5V ESP32) | Daya receiver |
| **GND (-)** | **GND** | Ground |

---

## 💻 Panduan Instalasi Step-by-Step

### Bagian A: Menjalankan Simulasi 3D di Webots (Laptop / PC)

1. **Install Webots di Ubuntu / Linux**:
   ```bash
   sudo snap install webots
   ```
2. **Install Dependensi Python Teleop**:
   ```bash
   pip3 install pygame
   ```
3. **Buka Lapangan Bola di Webots**:
   ```bash
   webots "/home/ambatron/MARUK(SOCCER)/worlds/soccer_field.wbt"
   ```
   *Tekan tombol **Play (▶️)** di toolbar atas Webots.*
4. **Jalankan Stasiun Teleop (Remote Control)**:
   Buka terminal baru:
   ```bash
   python3 "/home/ambatron/MARUK(SOCCER)/teleop/udp_sender.py"
   ```
   *Klik jendela Teleop untuk mengemudikan robot!*

---

### Bagian B: Flashing Firmware ke Hardware Fisik (ESP32)

1. **Buka Arduino IDE** di laptopmu.
2. Tambahkan URL Board ESP32 (jika belum pernah):
   * Buka **File** -> **Preferences**.
   * Di kolom *Additional Board Manager URLs*, masukkan:  
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Install Board ESP32:
   * Buka menu **Tools** -> **Board** -> **Boards Manager...**
   * Cari `esp32` oleh *Espressif Systems*, klik **Install**.
4. Buka Project Firmware:
   * Buka file: `/home/ambatron/MARUK(SOCCER)/firmware/MarukSoccer/MarukSoccer.ino`.
5. Pengaturan Board:
   * **Board**: Pilih `ESP32 Dev Module`.
   * **Port**: Pilih port USB ESP32-mu (misal `/dev/ttyUSB0`).
6. Klik tombol **Upload (Panah Kanan)**. Selesai!

---

## 🎮 Cara Mengemudikan Robot

### Menggunakan Remote Drone Fisik (Di Lapangan)
* **Stik Kanan (Pitch / Elevator)**: Dorong ke depan untuk **Maju**, tarik ke belakang untuk **Mundur**.
* **Stik Kanan (Roll) atau Stik Kiri (Yaw)**: Geser ke samping untuk **Belok / Muter di Tempat**.

### Menggunakan Keyboard (Di Simulasi Webots / Laptop)
* **`W` / `S`**: Maju / Mundur ($V_x$)
* **`A` / `D`**: Belok Kiri / Kanan ($\omega$)
* **`LSHIFT`**: Tahan untuk mode **Turbo Sprint (Gas 100%)**
* **`ESC`**: Keluar dari remote

---

## 🧠 Rumus Kinematika 2WD (Differential Drive)

$$V_{Kiri} = V_x + \omega$$
$$V_{Kanan} = V_x - \omega$$

Jika salah satu roda melebihi batas kecepatan maksimal (1.0), kedua roda dinormalisasi secara proporsional agar radius putaran robot tetap presisi tanpa meleset.
