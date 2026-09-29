#include <SPI.h>
#include <RH_NRF905.h>

RH_NRF905 radio(8, 9, 10);

const int POT_PIN = A0;

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("Запуск nRF905...");

  if (!radio.init()) {
    Serial.println("ERROR: nRF905 не инициализировался");
    while (1);
  }

  radio.setChannel(108);
  radio.setRF(RH_NRF905::TransmitPower10dBm);

  Serial.println("nRF905 готов");
}

void loop() {
  uint16_t pot = analogRead(POT_PIN);

  // На всякий случай выводим радио из зависшего TX-состояния
  radio.setModeIdle();

  bool ok = radio.send((uint8_t*)&pot, sizeof(pot));

  if (!ok) {
    Serial.println("Ошибка запуска передачи");
    delay(50);
    return;
  }

  // Ждём завершения передачи, но максимум 30 мс
  unsigned long start = millis();

  while (radio.isSending()) {
    if (millis() - start > 30) {
      Serial.println("TX TIMEOUT");
      radio.setModeIdle();
      break;
    }
  }

  Serial.print("Передано: ");
  Serial.println(pot);

  delay(100);
}