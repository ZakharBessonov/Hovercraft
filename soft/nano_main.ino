/*
  Nano ATmega328P 16 MHz + nRF905 + ONE-DIRECTION RC ESC on D2.
  UNO transmitter: keep the previously working 2-byte potentiometer sketch.
  Install RadioHead only. Do NOT include Servo.h or other .ino tabs from
  earlier versions in this sketch folder. No RH_ASK library editing is needed.

  ESC signal -> D2; ESC signal ground -> Nano GND.
  ESC main power -> battery; 3 motor wires -> ESC motor outputs.
  For first tests: REMOVE PROPELLER. Nano powered by USB;
  leave the ESC BEC red wire disconnected and insulated.
  Turn pot to zero, power UNO/Nano, then ESC; wait for its ready beeps.
  Serial: 115200. HOLD_ZERO -> hold received POT at 0..20 for 1 second.
  ARMED -> pot controls throttle. NO_LINK -> no valid packets.

  Assumes ESC calibrated for 1000..2000 us, approximately 50 Hz.
  Not for reversible/1500-us-neutral or DShot-only controllers.
  Boundaries may require calibration according to YOUR ESC manual.
  500 ms without a valid packet -> minimum throttle + re-arm required.
  This is throttle control, not closed-loop RPM control.

  Signal scheduling and arming/failsafe logic checked in a host simulation.
  No actual Arduino AVR build, oscilloscope or motor test was available.
*/
#include <SPI.h>
#include <RH_NRF905.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Select a classic Arduino Nano with ATmega328P, 16 MHz"
#endif

// NO Servo library is used. Timer1 Compare B drives D2; the unused
// RH_ASK Compare A handler may still be linked, but its interrupt is disabled.
// Do not instantiate RH_ASK or add Servo/timer libraries to this sketch.
const uint8_t ESC_PIN = 2;
// For a one-direction RC ESC with calibrated 1000..2000 us throttle.
const int ESC_MIN_US = 1000;
const int ESC_MAX_US = 2000;
const uint16_t POT_ZERO = 20;
const unsigned long LINK_TIMEOUT_MS = 500;
const unsigned long ARM_HOLD_MS = 1000;

// 16 MHz / 8 = 2 timer ticks per microsecond. A frame is 20,000 us.
volatile uint16_t escRequestedTicks = 2000;
uint16_t escFrameHighTicks = 2000;
bool escPinHigh = false;

void writeEscUs(uint16_t pulseUs) {
  if (pulseUs < ESC_MIN_US) pulseUs = ESC_MIN_US;
  if (pulseUs > ESC_MAX_US) pulseUs = ESC_MAX_US;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    escRequestedTicks = pulseUs * 2U;
  }
}

ISR(TIMER1_COMPB_vect) {
  if (!escPinHigh) {
    // Latch the width for the entire frame, so changes cannot change its period.
    escFrameHighTicks = escRequestedTicks;
    PORTD |= _BV(PD2);
    escPinHigh = true;
    OCR1B = static_cast<uint16_t>(OCR1B + escFrameHighTicks);
  } else {
    PORTD &= static_cast<uint8_t>(~_BV(PD2));
    escPinHigh = false;
    OCR1B = static_cast<uint16_t>(OCR1B + 40000U - escFrameHighTicks);
  }
}

void beginEsc() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    DDRD |= _BV(DDD2);
    PORTD &= static_cast<uint8_t>(~_BV(PD2));
    TCCR1A = 0; // Disconnect OC1A/OC1B: D9 and D10 remain regular radio pins.
    TCCR1B = 0;
    TCCR1C = 0;
    TIMSK1 = 0;
    TCNT1 = 0;
    escRequestedTicks = ESC_MIN_US * 2U;
    escFrameHighTicks = escRequestedTicks;
    escPinHigh = false;
    OCR1B = 200; // First minimum-throttle pulse after 100 us.
    TIFR1 = _BV(OCF1A) | _BV(OCF1B) | _BV(TOV1);
    TIMSK1 = _BV(OCIE1B); // Compare A is deliberately NOT enabled.
    TCCR1B = _BV(CS11); // Normal free-running timer; prescaler 8.
  }
}

// Constructor order: TRX_CE, TX_EN, CSN.
RH_NRF905 radio(8, 9, 10);
const uint16_t CHANNEL = 108;  // 433.2 MHz, low band
uint8_t networkAddress[] = {0x42, 0x4F, 0x41, 0x54};

void stopWithError(const __FlashStringHelper *message) {
  writeEscUs(ESC_MIN_US);
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

bool armed = false;
bool holdingZero = false;
bool havePacket = false;
unsigned long zeroSince = 0;
unsigned long lastPacket = 0;
unsigned long lastPrint = 0;
uint16_t latestPot = 0;
int outputUs = ESC_MIN_US;

void stopMotor() {
  armed = false;
  holdingZero = false;
  outputUs = ESC_MIN_US;
  writeEscUs(outputUs);
}

void checkLink() {
  if (havePacket && millis() - lastPacket >= LINK_TIMEOUT_MS) {
    stopMotor();
    havePacket = false;
    Serial.println(F("FAILSAFE: link lost; return pot to zero"));
  }
}

void acceptPot(uint16_t value) {
  // A bad/outdated packet must not keep the previous throttle running.
  if (value > 1023) return;
  latestPot = value;
  lastPacket = millis();
  havePacket = true;

  // At startup and after a link loss, hold zero for one second to enable.
  if (!armed) {
    writeEscUs(ESC_MIN_US);
    if (value <= POT_ZERO) {
      if (!holdingZero) {
        holdingZero = true;
        zeroSince = millis();
      }
      if (millis() - zeroSince >= ARM_HOLD_MS) {
        armed = true;
        Serial.println(F("ARMED"));
      }
    } else {
      holdingZero = false;
    }
    return;
  }

  // Small dead zone at the bottom; the rest maps to the ESC throttle range.
  outputUs = value <= POT_ZERO ? ESC_MIN_US :
    static_cast<int>(map(value, POT_ZERO, 1023, ESC_MIN_US, ESC_MAX_US));
  writeEscUs(outputUs);
}

void setup() {
  beginEsc();
  setupRadio();
  // Give a conventional aircraft ESC time to initialize at minimum throttle.
  delay(3000);
  radio.setModeRx();
  Serial.println(F("NANO ESC D2 ready: hold pot at zero for 1 second"));
}

void loop() {
  // Check before accepting a new packet: a recovered link must not auto-restart.
  checkLink();
  if (radio.available()) {
    uint8_t buffer[RH_NRF905_MAX_MESSAGE_LEN];
    uint8_t length = sizeof(buffer);
    if (radio.recv(buffer, &length)) {
      if (length == 2) {
        uint16_t value = static_cast<uint16_t>(buffer[0]) |
                         (static_cast<uint16_t>(buffer[1]) << 8);
        acceptPot(value);
      }
      radio.setModeRx();
    }
  }
  checkLink();
  if (millis() - lastPrint >= 250UL) {
    lastPrint = millis();
    Serial.print(F("POT=")); Serial.print(latestPot);
    Serial.print(F(" ESC_us=")); Serial.print(outputUs);
    Serial.print(F(" STATE="));
    if (!havePacket) Serial.println(F("NO_LINK"));
    else if (!armed) Serial.println(F("HOLD_ZERO"));
    else Serial.println(F("ARMED"));
  }
}
