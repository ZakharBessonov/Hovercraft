// Packet: C7 01 flags potLow potHigh angle. flags=0 during calibration, 1 when ready.
// Requires RadioHead. Classic UNO / Nano ATmega328P 16 MHz.
#include <SPI.h>
#include <RH_NRF905.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Select classic Arduino Nano, ATmega328P, 16 MHz"
#endif

const uint16_t ESC_MIN_US=1000;
const uint16_t ESC_MAX_US=2000;
// Nominal 90 +/-45 degrees for a conventional POSITION servo.
// Adjust these three pulse values to YOUR servo's actual measured angles.
const uint16_t SERVO_CENTER_US=1500;
const uint16_t SERVO_LEFT_US=1000;
const uint16_t SERVO_RIGHT_US=2000;
const uint16_t POT_ZERO=20;
const unsigned long LINK_TIMEOUT_MS=500;
const unsigned long ARM_HOLD_MS=1000;

// 2 ticks/us; sequence per 20ms frame: ESC high, servo high, both low.
// Timer1 Compare B avoids the unused RH_ASK Compare A ISR. No Servo library.
volatile uint16_t requestedEscTicks=ESC_MIN_US*2U;
volatile uint16_t requestedServoTicks=SERVO_CENTER_US*2U;
uint16_t frameEscTicks=ESC_MIN_US*2U;
uint16_t frameServoTicks=SERVO_CENTER_US*2U;
uint8_t pulsePhase=0;

void setOutputs(uint16_t escUs,uint16_t servoUs) {
  if(escUs<ESC_MIN_US) escUs=ESC_MIN_US;
  if(escUs>ESC_MAX_US) escUs=ESC_MAX_US;
  if(servoUs<500) servoUs=500;
  if(servoUs>2500) servoUs=2500;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    requestedEscTicks=escUs*2U;
    requestedServoTicks=servoUs*2U;
  }
}

ISR(TIMER1_COMPB_vect) {
  if(pulsePhase==0) {
    frameEscTicks=requestedEscTicks;
    frameServoTicks=requestedServoTicks;
    PORTD|=_BV(PD2);
    OCR1B=static_cast<uint16_t>(OCR1B+frameEscTicks);
    pulsePhase=1;
  } else if(pulsePhase==1) {
    PORTD&=static_cast<uint8_t>(~_BV(PD2));
    PORTD|=_BV(PD4);
    OCR1B=static_cast<uint16_t>(OCR1B+frameServoTicks);
    pulsePhase=2;
  } else {
    PORTD&=static_cast<uint8_t>(~_BV(PD4));
    OCR1B=static_cast<uint16_t>(OCR1B+40000U-frameEscTicks-frameServoTicks);
    pulsePhase=0;
  }
}

void beginOutputs() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    DDRD|=_BV(DDD2)|_BV(DDD4);
    PORTD&=static_cast<uint8_t>(~(_BV(PD2)|_BV(PD4)));
    TCCR1A=0; TCCR1B=0; TCCR1C=0; TIMSK1=0; TCNT1=0;
    requestedEscTicks=ESC_MIN_US*2U;
    requestedServoTicks=SERVO_CENTER_US*2U;
    pulsePhase=0;
    OCR1B=200;
    TIFR1=_BV(OCF1A)|_BV(OCF1B)|_BV(TOV1);
    TIMSK1=_BV(OCIE1B);
    TCCR1B=_BV(CS11);
  }
}

// Constructor order: TRX_CE, TX_EN, CSN.
RH_NRF905 radio(8, 9, 10);
const uint16_t CHANNEL = 108;  // 433.2 MHz, low band
uint8_t networkAddress[] = {0x42, 0x4F, 0x41, 0x54};

void stopWithError(const __FlashStringHelper *message) {
  setOutputs(ESC_MIN_US,SERVO_CENTER_US);
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


bool armed=false;
bool holdingZero=false;
bool havePacket=false;
bool remoteReady=false;
unsigned long zeroSince=0;
unsigned long lastPacket=0;
unsigned long lastPrint=0;
uint16_t latestPot=0;
uint8_t latestAngle=90;
uint16_t outputEscUs=ESC_MIN_US;
uint16_t outputServoUs=SERVO_CENTER_US;
uint32_t rejectedPackets=0;

void stopAll() {
  armed=false; holdingZero=false;
  outputEscUs=ESC_MIN_US; outputServoUs=SERVO_CENTER_US;
  latestAngle=90;
  setOutputs(outputEscUs,outputServoUs);
}

void checkLink() {
  if(havePacket && millis()-lastPacket>=LINK_TIMEOUT_MS) {
    stopAll(); havePacket=false; remoteReady=false;
    Serial.println(F("FAILSAFE: minimum throttle, rudder centered"));
  }
}

uint16_t angleToPulse(uint8_t angle) {
  if(angle<=90) return static_cast<uint16_t>(map(angle,45,90,SERVO_LEFT_US,SERVO_CENTER_US));
  return static_cast<uint16_t>(map(angle,90,135,SERVO_CENTER_US,SERVO_RIGHT_US));
}

void acceptControl(uint16_t pot,uint8_t angle,bool ready) {
  havePacket=true; lastPacket=millis(); remoteReady=ready; latestPot=pot;
  if(!ready) {stopAll();return;}
  latestAngle=angle;
  outputServoUs=angleToPulse(angle);
  if(!armed) {
    outputEscUs=ESC_MIN_US;
    if(pot<=POT_ZERO) {
      if(!holdingZero) {holdingZero=true;zeroSince=millis();}
      if(millis()-zeroSince>=ARM_HOLD_MS) {
        armed=true;Serial.println(F("ARMED"));
      }
    } else holdingZero=false;
  } else {
    outputEscUs=pot<=POT_ZERO ? ESC_MIN_US:
      static_cast<uint16_t>(map(pot,POT_ZERO,1023,ESC_MIN_US,ESC_MAX_US));
  }
  // Steering remains usable while throttle is waiting for zero.
  setOutputs(outputEscUs,outputServoUs);
}

void processPacket(const uint8_t* packet,uint8_t length) {
  if(length!=6 || packet[0]!=0xC7 || packet[1]!=1 || packet[2]>1) {
    ++rejectedPackets;return;
  }
  uint16_t pot=static_cast<uint16_t>(packet[3])|(static_cast<uint16_t>(packet[4])<<8);
  if(pot>1023 || packet[5]<45 || packet[5]>135) {++rejectedPackets;return;}
  acceptControl(pot,packet[5],packet[2]==1);
}

void setup() {
  beginOutputs();
  setupRadio();
  delay(3000); // Minimum throttle is generated by the timer during all startup delays.
  radio.setModeRx();
  Serial.println(F("RX ready: ESC D2, rudder D4. Load matching NEW transmitter"));
}

void loop() {
  checkLink(); // Recovered packets after a gap cannot auto-restart throttle.
  if(radio.available()) {
    uint8_t packet[RH_NRF905_MAX_MESSAGE_LEN];
    uint8_t length=sizeof(packet);
    if(radio.recv(packet,&length)) {
      processPacket(packet,length);
      radio.setModeRx();
    }
  }
  checkLink();
  if(millis()-lastPrint>=250UL) {
    lastPrint=millis();
    Serial.print(F("POT="));Serial.print(latestPot);
    Serial.print(F(" ESC_us="));Serial.print(outputEscUs);
    Serial.print(F(" ANGLE="));Serial.print(latestAngle);
    Serial.print(F(" SERVO_us="));Serial.print(outputServoUs);
    Serial.print(F(" BAD="));Serial.print(rejectedPackets);
    Serial.print(F(" STATE="));
    if(!havePacket) Serial.println(F("NO_LINK"));
    else if(!remoteReady) Serial.println(F("CALIBRATING"));
    else if(!armed) Serial.println(F("HOLD_ZERO"));
    else Serial.println(F("ARMED"));
  }
}