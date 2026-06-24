# Voyager 2026

Sistem kendali otomatis untuk wahana bawah air (AUV) berbasis Arduino Mega dan Raspberry Pi dengan kemampuan monitoring real-time sensor lingkungan.

## 🚀 Deskripsi Proyek

**Voyager 2026** adalah platform robotika bawah air yang dilengkapi dengan:
- **Kontrol diferensial drive** menggunakan 2 thruster
- **Monitoring sensor lingkungan** (pH, turbidity, jarak)
- **GPS navigation** untuk pelacakan posisi
- **Komunikasi real-time** antara Arduino Mega dan Raspberry Pi

Sistem ini dirancang untuk aplikasi eksplorasi laut, pengambilan sampel, dan monitoring kondisi air secara otomatis.

---

## 📋 Hardware Components

### Mikrokontroler & Komunikasi
- **Arduino Mega 2560** - Main controller
- **Raspberry Pi** - High-level processing & navigation
- **TinyGPS++ Module** - GPS tracking (Serial3, 9600 baud)

### Aktuator
- **2x Seaking V4 ESC** - Motor controller untuk thruster
- **2x Thruster (Kiri & Kanan)** - PWM pins 5 & 6

### Sensor
| Sensor | Pin | Fungsi |
|--------|-----|--------|
| Ultrasonic Front | D22/D23 | Deteksi jarak bahan depan |
| Ultrasonic Back | D24/D25 | Deteksi jarak bahan belakang |
| pH Sensor | A0 | Monitoring pH air |
| Turbidity Sensor | A1 | Monitoring kekeruhan air |

---

## 🔌 Pin Configuration

```
Digital Pins:
├─ D5  → PWM_ESC_1 (Thruster Kiri)
├─ D6  → PWM_ESC_2 (Thruster Kanan)
├─ D22 → TRIG_DEPAN (Ultrasonic)
├─ D23 → ECHO_DEPAN
├─ D24 → TRIG_BELAKANG
└─ D25 → ECHO_BELAKANG

Analog Pins:
├─ A0 → PH_PIN
└─ A1 → TURBIDITY_PIN

Serial:
├─ Serial  → Komunikasi Raspi (115200 baud)
└─ Serial3 → GPS Module (9600 baud)
```

---

## 🎮 Sistem Kontrol

### Differential Drive Mixing
Input dari Raspi berupa nilai **X (steering)** dan **Y (throttle)** dalam range `-1.0 hingga 1.0`:

```
Left Thruster  = Y + X
Right Thruster = Y - X
```

**Contoh Perintah Raspi:**
```
0.5,0.0    # Maju 50% kecepatan
0.0,0.5    # Belok kanan 50%
-0.3,0.7   # Maju-belok kiri
```

### PWM Range
- **Idle**: 1500 µs
- **Forward**: 1500 - 2000 µs
- **Reverse**: 1000 - 1500 µs
- **Speed Multiplier**: 500 (dapat disesuaikan di baris `SPEED_MULTIPLIER`)

### Soft-Start Ramping
Setiap perubahan PWM dilakukan secara bertahap (±10 µs per 50ms) untuk menghindari stress pada motor.

---

## 📊 Sensor Telemetry

Sistem mengirimkan data telemetri setiap **2 detik** ke Serial Monitor/Raspi:

```
========================================
Jarak Depan   : 45.30 cm
Status Depan  : SEDANG
Nilai pH      : 7.50
Turbidity Volt: 2.45

Joystick Input: X=0.50 | Y=0.75
Target PWM    : L=1550 | R=1450
Current PWM   : L=1545 | R=1450

Latitude      : -6.974720
Longitude     : 110.406200
========================================
```

### Status Kategori
- **KOSONG**: < 30% penuh
- **SEDANG**: 30-70% penuh
- **PENUH**: > 70% penuh

---

## 🔧 Instalasi & Setup

### 1. Install Dependencies
```bash
# Library TinyGPS++ dan Servo sudah built-in Arduino IDE
# Pastikan board = Arduino Mega 2560
```

### 2. Hardware Setup
1. Hubungkan ultrasonic sensor ke D22-D25
2. Hubungkan ESC ke PWM pins D5 & D6
3. Hubungkan pH & Turbidity sensor ke A0 & A1
4. Hubungkan GPS module ke Serial3 (RX3: D15, TX3: D14)
5. Hubungkan Raspi ke Serial (D0-RX, D1-TX)

### 3. Upload Code
```
Arduino IDE → Select Board: Mega 2560
           → Select Port: /dev/ttyUSB0 (atau COM port)
           → Upload
```

### 4. ESC Calibration
Saat startup, tunggu 4 detik untuk kalibrasi ESC Seaking V4.

---

## 📡 Komunikasi Serial

### Format Input dari Raspi
```
x_value,y_value\n
```
- Contoh: `0.3,0.7\n` (steering 0.3, throttle 0.7)

### Format Output ke Raspi
Telemetri dikirim dalam format teks terstruktur setiap 2 detik.

---

## ⚙️ Konfigurasi Parameter

Ubah nilai berikut di bagian `VARIABEL` untuk customisasi:

```cpp
// Container Height (cm)
const float TINGGI_TONG_DEPAN = 80.0;
const float TINGGI_TONG_BELAKANG = 80.0;

// Speed Control (0-500 untuk testing, max 500)
const int SPEED_MULTIPLIER = 500;
```

---

## 🚨 Troubleshooting

| Masalah | Solusi |
|---------|--------|
| GPS tidak ada sinyal | Pastikan antena GPS terpasang & berada di area terbuka |
| Thruster tidak merespons | Cek power ESC, calibration ulang, test PWM signal |
| Sensor tidak stabil | Verifikasi koneksi kabel, cek ADC value di Serial Monitor |
| Komunikasi Raspi terputus | Cek baud rate (115200), kabel USB, drivers CH340 |

---

## 📈 Performance Specs

- **Update Rate Sensor**: 2 detik
- **Control Loop Ramping**: 50 ms
- **Max Speed**: ~2.5 knots (tergantung thruster)
- **Operating Depth**: Sesuai rated depth thruster (tested hingga 50m)
- **Battery Life**: ~2-3 jam (tergantung beban motor)

---

## 📝 License

Proyek ini dibuat untuk keperluan akademik/robotika.

---

## 👨‍💻 Developer

**Fauzan** - Sistem Kontrol & Integration

---

## 📞 Dokumentasi Lebih Lanjut

- [TinyGPS++ Library](https://github.com/mikalhart/TinyGPS)
- [Arduino Servo Library](https://www.arduino.cc/en/Reference/Servo)
- [Seaking V4 ESC Manual](https://www.bluerobotics.com/store/thrusters/seaking-r3-thruster/)
