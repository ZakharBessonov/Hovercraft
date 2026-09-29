#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10);  // CE, CSN

Servo esc;

const byte address[6] = "MTR01";

const int ESC_PIN = 2;

uint16_t throttle = 1000;

unsigned long lastPacket = 0;

bool armed = false;

void setup() {
  Serial.begin(9600);

  // ESC
  esc.attach(ESC_PIN);

  // Сразу минимальный газ
  esc.writeMicroseconds(1000);

  Serial.println("ESC initialization...");

  // Даём ESC время пропищать и инициализироваться
  delay(5000);

  // Радио
  if (!radio.begin()) {
    Serial.println("ERROR: nRF24L01 не найден");
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openReadingPipe(0, address);
  radio.startListening();

  Serial.println("Радио готово");
  Serial.println("Поставь потенциометр UNO на минимум для запуска");
}

void loop() {

  if (radio.available()) {

    // Если накопилось несколько пакетов,
    // берём самый свежий
    while (radio.available()) {
      radio.read(&throttle, sizeof(throttle));
    }

    lastPacket = millis();

    throttle = constrain(throttle, 1000, 1600);

    // Защита от случайного запуска:
    // сначала обязательно нужно поставить потенциометр на минимум
    if (!armed) {

      esc.writeMicroseconds(1000);

      if (throttle <= 1050) {
        armed = true;
        Serial.println("ARMED! Мотор готов.");
      }

    } else {

      esc.writeMicroseconds(throttle);

      Serial.print("Throttle: ");
      Serial.println(throttle);
    }
  }


  // FAILSAFE:
  // если связь потеряна более чем на 0.5 секунды —
  // сразу останавливаем мотор
  if (millis() - lastPacket > 500) {

    esc.writeMicroseconds(1000);

    if (armed) {
      Serial.println("RADIO LOST -> MOTOR STOP");
    }

    armed = false;
  }
}