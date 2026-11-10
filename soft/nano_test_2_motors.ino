// код для проверки двух моторов которые стоят на D2 и D3 и больше на нано ничего нет.
// все работало на момент 27.10.26

#include <Servo.h>

Servo motor1;
Servo motor2;

const int MIN_SPEED = 1000;
const int MAX_SPEED = 1500;

void setMotors(int speed) {
  motor1.writeMicroseconds(speed);
  motor2.writeMicroseconds(speed);
}

void setup() {
  Serial.begin(115200);

  motor1.attach(2);  // Первый ESC — D2
  motor2.attach(3);  // Второй ESC — D3

  Serial.println("=== TWO MOTORS TEST ===");

  // Минимальный газ
  setMotors(MIN_SPEED);

  Serial.println("Waiting for ESC initialization...");
  delay(10000);

  // Плавный разгон
  Serial.println("STARTING MOTORS");

  for (int speed = MIN_SPEED; speed <= MAX_SPEED; speed += 5) {
    setMotors(speed);

    Serial.print("Motor 1 (D2): ");
    Serial.print(speed);

    Serial.print(" | Motor 2 (D3): ");
    Serial.println(speed);

    delay(100);
  }

  // Держим заданную скорость 5 секунд
  Serial.println("MAX SPEED REACHED");
  delay(5000);

  // Плавное торможение
  Serial.println("STOPPING MOTORS");

  for (int speed = MAX_SPEED; speed >= MIN_SPEED; speed -= 5) {
    setMotors(speed);

    Serial.print("Speed: ");
    Serial.println(speed);

    delay(100);
  }

  setMotors(MIN_SPEED);
  Serial.println("MOTORS STOPPED");
}

void loop() {
  // Оба мотора остановлены
}