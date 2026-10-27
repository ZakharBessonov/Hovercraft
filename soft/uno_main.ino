#include <SPI.h>
#include <RH_NRF905.h>
#include <string.h>

// Constructor order: TRX_CE, TX_EN, CSN.
RH_NRF905 radio(8, 9, 10);
const uint16_t CHANNEL = 108;  // 433.2 MHz, low band
uint8_t networkAddress[] = {0x42, 0x4F, 0x41, 0x54};

void stopWithError(const __FlashStringHelper *message) {
  Serial.println(message);
  while (true) { delay(1000); }
}

void setupRadio() {
  Serial.begin(115200);
  delay(1000);

  // PWR_UP must be connected to 3.3 V separately.
  if (!radio.init()) stopWithError(F("ERROR: radio.init"));
  radio.setModeIdle();

  // Verify the actual SPI write/read path; init() alone is not this test.
  uint8_t saved = radio.spiReadRegister(0);
  radio.spiWriteRegister(0, 0x5A);
  bool spiOK = (radio.spiReadRegister(0) == 0x5A);
  radio.spiWriteRegister(0, 0xA5);
  spiOK = (radio.spiReadRegister(0) == 0xA5) && spiOK;
  radio.spiWriteRegister(0, saved);
  if (!spiOK) stopWithError(F("ERROR: SPI write/read; check VCC, GND, CSN, MOSI, MISO, SCK"));

  radio.setChannel(CHANNEL, false);
  radio.setNetworkAddress(networkAddress, sizeof(networkAddress));
  // Low power is enough for a bench test with antennas, about 1 m apart.
  radio.setRF(RH_NRF905::TransmitPowerm10dBm);

  bool configOK = (radio.spiReadRegister(0) == 0x6C);
  for (uint8_t i = 0; i < sizeof(networkAddress); ++i) {
    configOK = (radio.spiReadRegister(5 + i) == networkAddress[i]) && configOK;
  }
  if (!configOK) stopWithError(F("ERROR: radio configuration readback"));
  Serial.println(F("SPI OK, REG0=0x6C, channel=108, band=433 MHz"));
}

const uint8_t POT_PIN = A0;

void setup() {
  setupRadio();
  pinMode(POT_PIN, INPUT);
  Serial.println(F("UNO potentiometer TX ready"));
}

void loop() {
  uint16_t potValue = analogRead(POT_PIN);
  // Two bytes, least significant byte first.
  uint8_t payload[2] = {
    static_cast<uint8_t>(potValue & 0xFF),
    static_cast<uint8_t>(potValue >> 8)
  };
  radio.setModeIdle();
  if (!radio.send(payload, sizeof(payload))) {
    Serial.println(F("ERROR: send()"));
    delay(50);
    return;
  }
  unsigned long started = millis();
  while (radio.isSending() && millis() - started < 500UL) {
    delay(1);
  }
  bool finished = !radio.isSending();
  radio.setModeIdle();
  if (finished) {
    Serial.print(F("TX POT="));
    Serial.println(potValue);
  } else {
    Serial.println(F("TX_TIMEOUT"));
  }
  delay(50);
}
