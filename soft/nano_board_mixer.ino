// Receiver for classic Arduino Nano ATmega328P, 16 MHz.
// Packet v4: C7 04 ready potLow potHigh steering forwardLow forwardHigh.
// D2 = lift ESC, D3 = LEFT rear ESC, D4 = RIGHT rear ESC (viewed from behind).
// All three ESCs use one-direction RC throttle: 1000 us stop, 2000 us full.
// RadioHead RH_NRF905 required.
#include <SPI.h>
#include <RH_NRF905.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Select classic Arduino Nano, ATmega328P, 16 MHz"
#endif

const uint16_t ESC_MIN_US = 1000;
const uint16_t ESC_MAX_US = 2000;
const uint16_t THROTTLE_ZERO = 20;
const uint16_t REAR_RAMP_US_PER_FRAME = 20; // Full ramp takes about 1 second.
const uint8_t PROTOCOL_VERSION = 4;
const unsigned long LINK_TIMEOUT_MS = 500;
const unsigned long ARM_HOLD_MS = 1000;

volatile uint16_t requestedLiftTicks = ESC_MIN_US * 2U;
volatile uint16_t requestedLeftTicks = ESC_MIN_US * 2U;
volatile uint16_t requestedRightTicks = ESC_MIN_US * 2U;
volatile uint16_t frameLiftTicks = ESC_MIN_US * 2U;
volatile uint16_t frameLeftTicks = ESC_MIN_US * 2U;
volatile uint16_t frameRightTicks = ESC_MIN_US * 2U;
uint8_t pulsePhase = 0;

void setOutputs(uint16_t liftUs, uint16_t leftUs, uint16_t rightUs) {
  liftUs = constrain(liftUs, ESC_MIN_US, ESC_MAX_US);
  leftUs = constrain(leftUs, ESC_MIN_US, ESC_MAX_US);
  rightUs = constrain(rightUs, ESC_MIN_US, ESC_MAX_US);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    requestedLiftTicks = liftUs * 2U;
    requestedLeftTicks = leftUs * 2U;
    requestedRightTicks = rightUs * 2U;
  }
}

uint16_t rampTicks(uint16_t current, uint16_t target) {
  const uint16_t step = REAR_RAMP_US_PER_FRAME * 2U;
  if (target > current && target - current > step) return current + step;
  if (current > target && current - target > step) return current - step;
  return target;
}

// Timer1 Compare B, prescaler 8 = 2 ticks/us. Frame = 20 ms (50 Hz).
ISR(TIMER1_COMPB_vect) {
  if (pulsePhase == 0) {
    frameLiftTicks = requestedLiftTicks;
    const uint16_t leftTarget = requestedLeftTicks;
    const uint16_t rightTarget = requestedRightTicks;
    if (leftTarget == ESC_MIN_US * 2U && rightTarget == ESC_MIN_US * 2U) {
      // Neutral, disarmed and failsafe stop both rear motors next frame.
      frameLeftTicks = leftTarget;
      frameRightTicks = rightTarget;
    } else {
      frameLeftTicks = rampTicks(frameLeftTicks, leftTarget);
      frameRightTicks = rampTicks(frameRightTicks, rightTarget);
    }
    PORTD |= _BV(PD2);
    OCR1B = static_cast<uint16_t>(OCR1B + frameLiftTicks);
    pulsePhase = 1;
  } else if (pulsePhase == 1) {
    PORTD &= static_cast<uint8_t>(~_BV(PD2));
    PORTD |= _BV(PD3);
    OCR1B = static_cast<uint16_t>(OCR1B + frameLeftTicks);
    pulsePhase = 2;
  } else if (pulsePhase == 2) {
    PORTD &= static_cast<uint8_t>(~_BV(PD3));
    PORTD |= _BV(PD4);
    OCR1B = static_cast<uint16_t>(OCR1B + frameRightTicks);
    pulsePhase = 3;
  } else {
    PORTD &= static_cast<uint8_t>(~_BV(PD4));
    OCR1B = static_cast<uint16_t>(
      OCR1B + 40000U - frameLiftTicks - frameLeftTicks - frameRightTicks
    );
    pulsePhase = 0;
  }
}

void beginOutputs() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    DDRD |= _BV(DDD2) | _BV(DDD3) | _BV(DDD4);
    PORTD &= static_cast<uint8_t>(~(_BV(PD2) | _BV(PD3) | _BV(PD4)));
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1C = 0;
    TIMSK1 = 0;
    TCNT1 = 0;
    requestedLiftTicks = frameLiftTicks = ESC_MIN_US * 2U;
    requestedLeftTicks = frameLeftTicks = ESC_MIN_US * 2U;
    requestedRightTicks = frameRightTicks = ESC_MIN_US * 2U;
    pulsePhase = 0;
    OCR1B = 200;
    TIFR1 = _BV(OCF1A) | _BV(OCF1B) | _BV(TOV1);
    TIMSK1 = _BV(OCIE1B);
    TCCR1B = _BV(CS11);
  }
}

// Constructor order: TRX_CE, TX_EN, CSN.
RH_NRF905 radio(8, 9, 10);
const uint16_t CHANNEL = 108;  // 433.2 MHz, low band
uint8_t networkAddress[] = {0x42, 0x4F, 0x41, 0x54};

void stopWithError(const __FlashStringHelper *message) {
  setOutputs(ESC_MIN_US, ESC_MIN_US, ESC_MIN_US);
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
bool remoteReady = false;
unsigned long zeroSince = 0;
unsigned long lastPacket = 0;
unsigned long lastPrint = 0;
uint16_t latestPot = 0;
uint16_t latestForward = 0;
uint8_t latestSteering = 90;
uint32_t rejectedPackets = 0;

void stopAll() {
  armed = false;
  holdingZero = false;
  setOutputs(ESC_MIN_US, ESC_MIN_US, ESC_MIN_US);
}

void checkLink() {
  if (havePacket && millis() - lastPacket >= LINK_TIMEOUT_MS) {
    stopAll();
    havePacket = false;
    remoteReady = false;
    Serial.println(F("FAILSAFE: all three motors minimum"));
  }
}

uint16_t throttleToPulse(uint16_t value) {
  if (value <= THROTTLE_ZERO) return ESC_MIN_US;
  return static_cast<uint16_t>(
    map(value, THROTTLE_ZERO, 1023, ESC_MIN_US, ESC_MAX_US)
  );
}

void mixRearMotors(uint16_t forward, uint8_t steering,
                   uint16_t &left, uint16_t &right) {
  // Positive steering = turn right = more thrust on the left.
  const int16_t turn = static_cast<int16_t>(
    (static_cast<long>(steering) - 90L) * 1023L / 45L
  );
  int16_t mixedLeft = static_cast<int16_t>(forward) + turn;
  int16_t mixedRight = static_cast<int16_t>(forward) - turn;
  if (mixedLeft < 0) mixedLeft = 0;
  if (mixedRight < 0) mixedRight = 0;

  const int16_t peak = max(mixedLeft, mixedRight);
  if (peak > 1023) {
    // Preserve the left/right ratio when one command exceeds full throttle.
    mixedLeft = static_cast<long>(mixedLeft) * 1023L / peak;
    mixedRight = static_cast<long>(mixedRight) * 1023L / peak;
  }
  left = static_cast<uint16_t>(mixedLeft);
  right = static_cast<uint16_t>(mixedRight);
}

void acceptControl(uint16_t pot, uint16_t forward, uint8_t steering, bool ready) {
  havePacket = true;
  lastPacket = millis();
  remoteReady = ready;
  latestPot = pot;
  latestForward = forward;
  latestSteering = steering;
  if (!ready) {
    stopAll();
    return;
  }

  uint16_t left = 0;
  uint16_t right = 0;
  mixRearMotors(forward, steering, left, right);

  if (!armed) {
    setOutputs(ESC_MIN_US, ESC_MIN_US, ESC_MIN_US);
    // A horizontal stick command now drives a motor too: arm only in neutral.
    if (pot <= THROTTLE_ZERO && forward <= THROTTLE_ZERO && steering == 90) {
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

  setOutputs(throttleToPulse(pot), throttleToPulse(left), throttleToPulse(right));
}

void processPacket(const uint8_t *packet, uint8_t length) {
  if (length != 8 || packet[0] != 0xC7 ||
      packet[1] != PROTOCOL_VERSION || packet[2] > 1) {
    ++rejectedPackets;
    return;
  }
  const uint16_t pot = static_cast<uint16_t>(packet[3]) |
                       (static_cast<uint16_t>(packet[4]) << 8);
  const uint16_t forward = static_cast<uint16_t>(packet[6]) |
                           (static_cast<uint16_t>(packet[7]) << 8);
  if (pot > 1023 || forward > 1023 || packet[5] < 45 || packet[5] > 135) {
    ++rejectedPackets;
    return;
  }
  acceptControl(pot, forward, packet[5], packet[2] == 1);
}

void setup() {
  beginOutputs();
  setupRadio();
  delay(3000);
  radio.setModeRx();
  Serial.println(F("RX mixer: lift D2; LEFT rear D3; RIGHT rear D4"));
  Serial.println(F("Release joystick and turn POT to zero for 1 second"));
}

void loop() {
  checkLink();
  if (radio.available()) {
    uint8_t packet[RH_NRF905_MAX_MESSAGE_LEN];
    uint8_t length = sizeof(packet);
    if (radio.recv(packet, &length)) {
      processPacket(packet, length);
      radio.setModeRx();
    }
  }
  checkLink();

  if (millis() - lastPrint >= 250UL) {
    lastPrint = millis();
    uint16_t liftUs, leftUs, rightUs;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      liftUs = frameLiftTicks / 2U;
      leftUs = frameLeftTicks / 2U;
      rightUs = frameRightTicks / 2U;
    }
    Serial.print(F("POT=")); Serial.print(latestPot);
    Serial.print(F(" THR=")); Serial.print(latestForward);
    Serial.print(F(" STEER=")); Serial.print(static_cast<int>(latestSteering) - 90);
    Serial.print(F(" D2_us=")); Serial.print(liftUs);
    Serial.print(F(" LEFT_D3_us=")); Serial.print(leftUs);
    Serial.print(F(" RIGHT_D4_us=")); Serial.print(rightUs);
    Serial.print(F(" BAD=")); Serial.print(rejectedPackets);
    Serial.print(F(" STATE="));
    if (!havePacket) Serial.println(F("NO_LINK"));
    else if (!remoteReady) Serial.println(F("CALIBRATING"));
    else if (!armed) Serial.println(F("HOLD_ZERO"));
    else Serial.println(F("ARMED"));
  }
}
