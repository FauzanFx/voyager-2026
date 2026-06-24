#include <Servo.h>

Servo esckanan;
Servo esckiri;

#define ESC_KANAN_PIN 3
#define ESC_KIRI_PIN 5

void setup() {
  esckanan.attach(ESC_KANAN_PIN);
  esckiri.attach(ESC_KIRI_PIN);
    // Jalan
  esckanan.writeMicroseconds(1200);
  esckiri.writeMicroseconds(1200);
  delay(10000);
}

void loop() {

  // Stop
  esckanan.writeMicroseconds(1500);
  esckiri.writeMicroseconds(1500);
  delay(5000);
}