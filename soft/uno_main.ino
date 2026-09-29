#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 10);  // CE, CSN

const byte address[6] = "MTR01";

const int POT_PIN = A0;

void setup() {
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println("ERROR:ddcn n nRF24L01 не найден");
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("Передатчик готов");
}

void loop() {
  int pot = analogRead(POT_PIN);

  // 0 положения потенциометра = стоп
  // 1023 = максимальный разрешённый газ
  uint16_t throttle = map(pot, 0, 1023, 1000, 1600);

  bool success = radio.write(&throttle, sizeof(throttle));

  Serial.print("Pot: ");
  Serial.print(pot);

  Serial.print("  ESC: ");
  Serial.print(throttle);

  Serial.print("  Radio: ");
  Serial.println(success ? "OK" : "FAIL");

  delay(50);
}