#include <Servo.h>

Servo myServo;

void setup() {
  Serial.begin(115200);

  myServo.attach(4);  // Сигнал на D4

  Serial.println("SERVO TEST START");

  myServo.write(90);
  delay(2000);
}

void loop() {
  Serial.println("SERVO: 60 degrees");
  myServo.write(60);
  delay(1500);

  Serial.println("SERVO: 120 degrees");
  myServo.write(120);
  delay(1500);
}