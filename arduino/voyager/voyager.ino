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

// --- Variabel pH ---
float calibration_value = 15.7;
int buffer_arr[10], temp;
float ph_act;

// --- Variabel Turbidity ---
float turbidity_ntu = 0;

// ================= VARIABEL THRUSTER =================
int currentPwmKiri = 1500;
int targetPwmKiri  = 1500;

int currentPwmKanan = 1500;
int targetPwmKanan  = 1500;

float x = 0.0; // Steering
float y = 0.0; // Throttle

const int SPEED_MULTIPLIER = 500; 

// ================= TIMER =================
unsigned long waktuPrintTerakhir = 0; 
unsigned long waktuRampingTerakhir = 0; 

// ================= FUNGSI SENSOR ULTRASONIK =================
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

// ================= FUNGSI SENSOR KUALITAS AIR =================
void bacaPH() {
  unsigned long int avgval;
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_PIN);
    delayMicroseconds(100);
  }
  // Sorting array
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }
  // Ambil rata-rata 6 nilai tengah (buang 2 terendah, 2 tertinggi)
  avgval = 0;
  for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
  float volt_ph = (float)avgval * ((5.0 / 1023.0) / 6.0);
  ph_act = -7.50 * volt_ph + calibration_value;
}

float bacaTurbidity() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(TURBIDITY_PIN);
    delayMicroseconds(100);
  }
  float avg = sum / 10.0;
  float volt = avg * (5.0 / 1023.0);
  float ntu = 0;
  
  if (volt < 2.5) {
    ntu = 3000;
  } else {
    ntu = -1120.4 * sq(volt) + 5742.3 * volt - 4352.9;
    if (ntu < 0) ntu = 0;
  }
  return ntu;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);  
  Serial3.begin(9600);   

  thrusterKiri.attach(PWM_ESC_1);
  thrusterKanan.attach(PWM_ESC_2);
  
  thrusterKiri.writeMicroseconds(1500);
  thrusterKanan.writeMicroseconds(1500);
  delay(4000); 

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
      x = dataMasuk.substring(0, commaIndex).toFloat();
      y = dataMasuk.substring(commaIndex + 1).toFloat();

      // --- DIFFERENTIAL DRIVE MIXING ---
      float leftMix  = constrain(y + x, -1.0, 1.0);
      float rightMix = constrain(y - x, -1.0, 1.0);

      // Hitung Target PWM Kiri (Standar)
      targetPwmKiri = 1500 + (leftMix * SPEED_MULTIPLIER);
      
      // Hitung Target PWM Kanan RAW
      int pwmKananRaw = 1500 + (rightMix * SPEED_MULTIPLIER);
      
      // --- MIRRORING PROPELLER KANAN (CCW) ---
      targetPwmKanan = (2 * 1500) - pwmKananRaw;

      // Pastikan tetap berada di rentang batas aman ESC
      targetPwmKiri  = constrain(targetPwmKiri, 1000, 2000);
      targetPwmKanan = constrain(targetPwmKanan, 1000, 2000);
    }
  }

  // 3. ================= RAMPING (SOFT-START) 50ms =================
  if (millis() - waktuRampingTerakhir >= 50) {
    waktuRampingTerakhir = millis();

    if (currentPwmKiri < targetPwmKiri) {
      currentPwmKiri += 10;
      if (currentPwmKiri > targetPwmKiri) currentPwmKiri = targetPwmKiri;
    } else if (currentPwmKiri > targetPwmKiri) {
      currentPwmKiri -= 10;
      if (currentPwmKiri < targetPwmKiri) currentPwmKiri = targetPwmKiri;
    }

    if (currentPwmKanan < targetPwmKanan) {
      currentPwmKanan += 10;
      if (currentPwmKanan > targetPwmKanan) currentPwmKanan = targetPwmKanan;
    } else if (currentPwmKanan > targetPwmKanan) {
      currentPwmKanan -= 10;
      if (currentPwmKanan < targetPwmKanan) currentPwmKanan = targetPwmKanan;
    }

    thrusterKiri.writeMicroseconds(currentPwmKiri);
    thrusterKanan.writeMicroseconds(currentPwmKanan);
  }

  // 4. ================= TELEMETRI SENSOR (2 Detik) =================
  if (millis() - waktuPrintTerakhir >= 2000) {
    waktuPrintTerakhir = millis(); 

    // Update data kualitas air
    bacaPH();
    turbidity_ntu = bacaTurbidity();

    // Baca Jarak
    float jarakDepan = bacaJarak(TRIG_DEPAN, ECHO_DEPAN);
    float jarakBelakang = bacaJarak(TRIG_BELAKANG, ECHO_BELAKANG);
    float penuhDepan = hitungPersenPenuh(jarakDepan, TINGGI_TONG_DEPAN);
    float penuhBelakang = hitungPersenPenuh(jarakBelakang, TINGGI_TONG_BELAKANG);
    float totalKepenuhan = (penuhDepan + penuhBelakang) / 2.0;

    Serial.println("\n========================================");
    Serial.print("Jarak Depan   : "); Serial.print(jarakDepan); Serial.println(" cm");
    Serial.print("Status Depan  : "); Serial.println(statusTong(penuhDepan));
    Serial.print("Jarak Belakang: "); Serial.print(jarakBelakang); Serial.println(" cm");
    Serial.print("Status Belakang: "); Serial.println(statusTong(penuhBelakang));
    
    Serial.print("TOTAL KEPENUHAN : "); Serial.print(totalKepenuhan); Serial.println(" %");
    
    Serial.print("Nilai pH      : "); Serial.println(ph_act, 2);
    Serial.print("Turbidity NTU : "); Serial.println(turbidity_ntu, 1);
    
    Serial.println("----------------------------------------");
    Serial.print("Joystick Input: X="); Serial.print(x, 2); Serial.print(" | Y="); Serial.println(y, 2);
    Serial.print("Target PWM    : L="); Serial.print(targetPwmKiri); Serial.print(" | R="); Serial.println(targetPwmKanan);
    Serial.print("Current PWM   : L="); Serial.print(currentPwmKiri); Serial.print(" | R="); Serial.println(currentPwmKanan);

    if (gps.location.isValid()) {
      Serial.print("Latitude      : "); Serial.println(gps.location.lat(), 6);
      Serial.print("Longitude     : "); Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("GPS           : Menunggu sinyal...");
    }
    Serial.println("========================================");
  }
}