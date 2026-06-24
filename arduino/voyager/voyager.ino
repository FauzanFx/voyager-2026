#include <TinyGPS++.h>
#include <Servo.h>

// ================= PIN =================
#define TRIG_DEPAN      22
#define ECHO_DEPAN      23

#define TRIG_BELAKANG   24
#define ECHO_BELAKANG   25

#define PH_PIN          A0
#define TURBIDITY_PIN   A1

#define PWM_ESC_1       5
#define PWM_ESC_2       6

// ================= OBJEK =================
TinyGPSPlus gps;
Servo thrusterKiri;
Servo thrusterKanan; 

// ================= VARIABEL SENSOR =================
const float TINGGI_TONG_DEPAN = 80.0;      // cm
const float TINGGI_TONG_BELAKANG = 80.0;   // cm

// ================= VARIABEL THRUSTER =================
int currentPwmKiri = 1500;
int targetPwmKiri  = 1500;

int currentPwmKanan = 1500;
int targetPwmKanan  = 1500;

float x = 0.0; // Steering
float y = 0.0; // Throttle

// Batas perkalian PWM (Jika 1.0 = 500, maka rentang menjadi 1000 - 2000)
// Gunakan 250 untuk testing awal agar motor tidak terlalu agresif (Max 1250 - 1750)
const int SPEED_MULTIPLIER = 500; 

// ================= TIMER =================
unsigned long waktuPrintTerakhir = 0; 
unsigned long waktuRampingTerakhir = 0; 

// ================= FUNGSI SENSOR =================
float bacaJarak(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if(duration == 0) return -1;
  return duration * 0.0343 / 2.0;
}

float hitungPersenPenuh(float jarak, float tinggiTong) {
  if(jarak < 0) return 0;
  float persen = ((tinggiTong - jarak) / tinggiTong) * 100.0;
  return constrain(persen, 0, 100);
}

String statusTong(float persen) {
  if(persen < 30) return "KOSONG";
  if(persen < 70) return "SEDANG";
  return "PENUH";
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);  // Komunikasi dengan Raspi
  Serial3.begin(9600);   // Modul GPS

  // Inisialisasi ESC
  thrusterKiri.attach(PWM_ESC_1);
  thrusterKanan.attach(PWM_ESC_2);
  
  // Arming Seaking V4 
  thrusterKiri.writeMicroseconds(1500);
  thrusterKanan.writeMicroseconds(1500);
  delay(4000); // Tunggu kalibrasi ESC

  pinMode(TRIG_DEPAN, OUTPUT);
  pinMode(ECHO_DEPAN, INPUT);
  pinMode(TRIG_BELAKANG, OUTPUT);
  pinMode(ECHO_BELAKANG, INPUT);

  Serial.println("SISTEM WAHANA SIAP (MEGA + RASPI)");
  Serial.println("Format Serial dari Raspi: x,y");
}

// ================= LOOP =================
void loop() {
  // 1. ================= BACA GPS =================
  while (Serial3.available()) {
    gps.encode(Serial3.read());
  }

  // 2. ================= BACA PERINTAH RASPI & MIXING =================
  if (Serial.available() > 0) {
    String dataMasuk = Serial.readStringUntil('\n');
    int commaIndex = dataMasuk.indexOf(',');

    if (commaIndex > 0) {
      // Parsing nilai X dan Y (-1.0 s.d 1.0)
      x = dataMasuk.substring(0, commaIndex).toFloat();
      y = dataMasuk.substring(commaIndex + 1).toFloat();

      x = constrain(x, -1.0, 1.0);
      y = constrain(y, -1.0, 1.0);

      // --- DIFFERENTIAL DRIVE MIXING ---
      float leftMix  = y + x;
      float rightMix = y - x;

      leftMix  = constrain(leftMix, -1.0, 1.0);
      rightMix = constrain(rightMix, -1.0, 1.0);

      // Konversi ke Target PWM (1500 +/- Multiplier)
      targetPwmKiri  = 1500 + (leftMix * SPEED_MULTIPLIER);
      targetPwmKanan = 1500 + (rightMix * SPEED_MULTIPLIER);
    }
  }

  // 3. ================= RAMPING (SOFT-START) 50ms =================
  if (millis() - waktuRampingTerakhir >= 50) {
    waktuRampingTerakhir = millis();

    // Ramping Thruster Kiri
    if (currentPwmKiri < targetPwmKiri) {
      currentPwmKiri += 10;
      if (currentPwmKiri > targetPwmKiri) currentPwmKiri = targetPwmKiri;
    } else if (currentPwmKiri > targetPwmKiri) {
      currentPwmKiri -= 10;
      if (currentPwmKiri < targetPwmKiri) currentPwmKiri = targetPwmKiri;
    }

    // Ramping Thruster Kanan
    if (currentPwmKanan < targetPwmKanan) {
      currentPwmKanan += 10;
      if (currentPwmKanan > targetPwmKanan) currentPwmKanan = targetPwmKanan;
    } else if (currentPwmKanan > targetPwmKanan) {
      currentPwmKanan -= 10;
      if (currentPwmKanan < targetPwmKanan) currentPwmKanan = targetPwmKanan;
    }

    // Terapkan PWM ke ESC
    thrusterKiri.writeMicroseconds(currentPwmKiri);
    thrusterKanan.writeMicroseconds(currentPwmKanan);
  }

  // 4. ================= TELEMETRI SENSOR (2 Detik) =================
  if (millis() - waktuPrintTerakhir >= 2000) {
    waktuPrintTerakhir = millis(); 

    // Baca Sensor
    float jarakDepan = bacaJarak(TRIG_DEPAN, ECHO_DEPAN);
    float jarakBelakang = bacaJarak(TRIG_BELAKANG, ECHO_BELAKANG);
    float penuhDepan = hitungPersenPenuh(jarakDepan, TINGGI_TONG_DEPAN);
    float penuhBelakang = hitungPersenPenuh(jarakBelakang, TINGGI_TONG_BELAKANG);
    float totalKepenuhan = (penuhDepan + penuhBelakang) / 2.0;

    int phADC = analogRead(PH_PIN);
    float phValue = 3.5 * (phADC * (5.0 / 1023.0));
    
    int turbADC = analogRead(TURBIDITY_PIN);
    float turbVoltage = turbADC * (5.0 / 1023.0);

    // Output ke Serial Monitor (atau ditangkap ulang oleh Raspi)
    Serial.println("\n========================================");
    Serial.print("Jarak Depan   : "); Serial.print(jarakDepan); Serial.println(" cm");
    Serial.print("Status Depan  : "); Serial.println(statusTong(penuhDepan));
    Serial.print("Nilai pH      : "); Serial.println(phValue, 2);
    Serial.print("Turbidity Volt: "); Serial.println(turbVoltage, 2);
    
    Serial.println("----------------------------------------");
    Serial.print("Joystick Input: X="); Serial.print(x, 2); Serial.print(" | Y="); Serial.println(y, 2);
    Serial.print("Target PWM    : L="); Serial.print(targetPwmKiri); Serial.print(" | R="); Serial.println(targetPwmKanan);
    Serial.print("Current PWM   : L="); Serial.print(currentPwmKiri); Serial.print(" | R="); Serial.println(currentPwmKanan);

    // GPS
    if (gps.location.isValid()) {
      Serial.print("Latitude      : "); Serial.println(gps.location.lat(), 6);
      Serial.print("Longitude     : "); Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("GPS           : Menunggu sinyal...");
    }
    Serial.println("========================================");
  }
}