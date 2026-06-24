#include <TinyGPS++.h>

// ================= PIN =================
#define TRIG_DEPAN      22
#define ECHO_DEPAN      23

#define TRIG_BELAKANG   24
#define ECHO_BELAKANG   25

#define PH_PIN          A0
#define TURBIDITY_PIN   A1

// ================= GPS =================
TinyGPSPlus gps;

// ================= TINGGI TONG =================
const float TINGGI_TONG_DEPAN = 80.0;      // cm
const float TINGGI_TONG_BELAKANG = 80.0;   // cm

// ==============================================
float bacaJarak(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if(duration == 0)
  {
    return -1;
  }

  float jarak = duration * 0.0343 / 2.0;

  return jarak;
}

// ==============================================
float hitungPersenPenuh(float jarak, float tinggiTong)
{
  if(jarak < 0) return 0;

  float persen = ((tinggiTong - jarak) / tinggiTong) * 100.0;

  persen = constrain(persen, 0, 100);

  return persen;
}

// ==============================================
String statusTong(float persen)
{
  if(persen < 30)
    return "KOSONG";

  if(persen < 70)
    return "SEDANG";

  return "PENUH";
}

// ==============================================
void setup()
{
  Serial.begin(115200);

  Serial3.begin(9600);   // GPS

  pinMode(TRIG_DEPAN, OUTPUT);
  pinMode(ECHO_DEPAN, INPUT);

  pinMode(TRIG_BELAKANG, OUTPUT);
  pinMode(ECHO_BELAKANG, INPUT);

  Serial.println("SISTEM MONITORING SMART BIN");
}

// ==============================================
void loop()
{
  // ================= GPS =================
  while (Serial3.available())
  {
    gps.encode(Serial3.read());
  }

  // ================= ULTRASONIK =================
  float jarakDepan = bacaJarak(TRIG_DEPAN, ECHO_DEPAN);
  float jarakBelakang = bacaJarak(TRIG_BELAKANG, ECHO_BELAKANG);

  float penuhDepan =
      hitungPersenPenuh(jarakDepan, TINGGI_TONG_DEPAN);

  float penuhBelakang =
      hitungPersenPenuh(jarakBelakang, TINGGI_TONG_BELAKANG);

  float totalKepenuhan =
      (penuhDepan + penuhBelakang) / 2.0;

  // ================= pH =================
  int phADC = analogRead(PH_PIN);

  float phVoltage =
      phADC * (5.0 / 1023.0);

  // Rumus sementara (harus dikalibrasi)
  float phValue =
      3.5 * phVoltage;

  // ================= TURBIDITY =================
  int turbADC = analogRead(TURBIDITY_PIN);

  float turbVoltage =
      turbADC * (5.0 / 1023.0);

  // ================= OUTPUT =================
  Serial.println();
  Serial.println("========================================");
  Serial.println("      SMART BIN MONITORING SYSTEM");
  Serial.println("========================================");

  // Tong depan
  Serial.println("TONG DEPAN");
  Serial.print("Jarak       : ");
  Serial.print(jarakDepan);
  Serial.println(" cm");

  Serial.print("Kepenuhan   : ");
  Serial.print(penuhDepan);
  Serial.println(" %");

  Serial.print("Status      : ");
  Serial.println(statusTong(penuhDepan));

  Serial.println();

  // Tong belakang
  Serial.println("TONG BELAKANG");
  Serial.print("Jarak       : ");
  Serial.print(jarakBelakang);
  Serial.println(" cm");

  Serial.print("Kepenuhan   : ");
  Serial.print(penuhBelakang);
  Serial.println(" %");

  Serial.print("Status      : ");
  Serial.println(statusTong(penuhBelakang));

  Serial.println();

  // Total
  Serial.print("TOTAL KEPENUHAN : ");
  Serial.print(totalKepenuhan);
  Serial.println(" %");

  Serial.println();

  // pH
  Serial.print("Nilai pH        : ");
  Serial.println(phValue, 2);

  // Turbidity
  Serial.print("Turbidity Volt  : ");
  Serial.println(turbVoltage, 2);

  Serial.println();

  // GPS
  if (gps.location.isValid())
  {
    Serial.print("Latitude  : ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude : ");
    Serial.println(gps.location.lng(), 6);
  }
  else
  {
    Serial.println("GPS : Menunggu sinyal...");
  }

  Serial.println("========================================");

  delay(2000);
}