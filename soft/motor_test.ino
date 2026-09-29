#include <Servo.h>

Servo esc;

void setup() {
  esc.attach(2);

  // Минимальный газ
  esc.writeMicroseconds(1000);

  // Ждём, пока ESC пропищит и заармится
  delay(8000);

  // Небольшой газ
  esc.writeMicroseconds(1800);
}

void loop() {
  esc.writeMicroseconds(1500);
}