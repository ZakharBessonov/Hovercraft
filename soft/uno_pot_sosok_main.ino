// Packet: C7 01 flags potLow potHigh angle. flags=0 during calibration, 1 when ready.
// Requires RadioHead. Classic UNO / Nano ATmega328P 16 MHz.
#include <SPI.h>
#include <RH_NRF905.h>
#include <EEPROM.h>

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


// Connect shield A0..A3 to UNO A0..A3, 5V to 5V, GND to GND.
// IMPORTANT: move the external throttle potentiometer wiper from A0 to A4.
const uint8_t THROTTLE_PIN = A4;
const uint8_t AXIS_PINS[4] = {A0, A1, A2, A3};
const int DEAD_ZONE = 25;
const bool REVERSE_STEERING = false; // true if the installed rudder turns backwards
const int EEPROM_BASE = 32;
uint16_t axes[4] = {512,512,512,512};
uint16_t centers[4] = {512,512,512,512};
uint8_t selectedAxis = 0;
int8_t rightSign = 1;
uint16_t steeringSpan = 450;
bool configured = false;
int8_t candidate = -1;
int8_t candidateSign = 1;
uint16_t candidatePeak = 0;
unsigned long candidateSince = 0;
unsigned long lastPrint = 0;

uint16_t readAnalogSettled(uint8_t pin) {
  analogRead(pin); // discard first sample after ADC multiplexer changes
  return static_cast<uint16_t>(analogRead(pin));
}

void readAxes() {
  for(uint8_t i=0; i<4; ++i) axes[i] = readAnalogSettled(AXIS_PINS[i]);
}

void captureCenters() {
  uint32_t sums[4] = {0,0,0,0};
  for(uint8_t sample=0; sample<32; ++sample) {
    readAxes();
    for(uint8_t i=0; i<4; ++i) sums[i] += axes[i];
    delay(2);
  }
  for(uint8_t i=0; i<4; ++i) centers[i] = sums[i]/32;
}

bool loadCalibration() {
  uint8_t data[8];
  uint8_t check=0x5A;
  for(uint8_t i=0; i<8; ++i) data[i]=EEPROM.read(EEPROM_BASE+i);
  for(uint8_t i=0; i<7; ++i) check ^= data[i];
  uint16_t span=static_cast<uint16_t>(data[5]) | (static_cast<uint16_t>(data[6])<<8);
  if(data[0]!=0xB4 || data[1]!=0x71 || data[2]!=1 || data[3]>3 ||
     data[4]>1 || data[7]!=check || span<200 || span>800) return false;
  selectedAxis=data[3];
  rightSign=data[4] ? 1 : -1;
  steeringSpan=span;
  return true;
}

void saveCalibration() {
  uint8_t data[8]={0xB4,0x71,1,selectedAxis,
    static_cast<uint8_t>(rightSign>0 ? 1:0),
    static_cast<uint8_t>(steeringSpan),static_cast<uint8_t>(steeringSpan>>8),0x5A};
  for(uint8_t i=0; i<7; ++i) data[7]^=data[i];
  EEPROM.update(EEPROM_BASE,0); // invalidate before updating
  for(uint8_t i=1; i<8; ++i) EEPROM.update(EEPROM_BASE+i,data[i]);
  EEPROM.update(EEPROM_BASE,data[0]);
}

void requestCalibration() {
  configured=false;
  candidate=-1;
  Serial.println(F("CENTER: release BOTH sticks now; keep throttle at zero"));
  delay(1000);
  captureCenters();
  Serial.println(F("CAL: move ONLY RIGHT stick RIGHT to its stop; hold 1 second"));
}

void updateCalibration() {
  int16_t bestMagnitude=0;
  int8_t bestAxis=-1;
  int8_t sign=1;
  for(uint8_t i=0; i<4; ++i) {
    int16_t delta=static_cast<int16_t>(axes[i])-centers[i];
    int16_t magnitude=delta<0 ? -delta:delta;
    if(magnitude>bestMagnitude) {
      bestMagnitude=magnitude; bestAxis=i; sign=delta>=0 ? 1:-1;
    }
  }
  if(bestMagnitude<200) { candidate=-1; return; }
  if(candidate!=bestAxis || candidateSign!=sign) {
    candidate=bestAxis; candidateSign=sign; candidatePeak=bestMagnitude;
    candidateSince=millis();
    return;
  }
  if(bestMagnitude>candidatePeak) candidatePeak=bestMagnitude;
  if(millis()-candidateSince<800UL) return;
  selectedAxis=static_cast<uint8_t>(candidate);
  rightSign=candidateSign;
  steeringSpan=candidatePeak>800 ? 800:candidatePeak;
  saveCalibration();
  configured=true;
  Serial.print(F("CAL_SAVED: steering=A")); Serial.print(selectedAxis);
  Serial.println(F("; release stick. Calibration remembered. C = recalibrate"));
}

uint8_t steeringAngle() {
  int32_t delta=(static_cast<int32_t>(axes[selectedAxis])-centers[selectedAxis])*rightSign;
  if(REVERSE_STEERING) delta=-delta;
  int32_t magnitude=delta<0 ? -delta:delta;
  if(magnitude<=DEAD_ZONE) return 90;
  if(magnitude>steeringSpan) magnitude=steeringSpan;
  int offset=(magnitude-DEAD_ZONE)*45L/(steeringSpan-DEAD_ZONE);
  return static_cast<uint8_t>(90+(delta<0 ? -offset:offset));
}

void setup() {
  setupRadio();
  for(uint8_t i=0; i<4; ++i) pinMode(AXIS_PINS[i],INPUT);
  pinMode(THROTTLE_PIN,INPUT);
  Serial.println(F("Release both sticks during boot; throttle is on A4"));
  configured=loadCalibration();
  if(configured) {
    captureCenters();
    Serial.println(F("READY: stored axis loaded. Send C with sticks released to recalibrate"));
  } else requestCalibration();
}

void loop() {
  if(Serial.available()) {
    char c=Serial.read();
    if(c=='c' || c=='C') requestCalibration();
  }
  readAxes();
  if(!configured) updateCalibration();
  uint16_t pot=readAnalogSettled(THROTTLE_PIN);
  uint8_t angle=configured ? steeringAngle():90;
  uint16_t transmittedPot=configured ? pot:0;
  uint8_t packet[6]={0xC7,1,static_cast<uint8_t>(configured ? 1:0),
    static_cast<uint8_t>(transmittedPot),static_cast<uint8_t>(transmittedPot>>8),angle};
  radio.setModeIdle();
  bool sent=radio.send(packet,sizeof(packet));
  if(sent) {
    unsigned long started=millis();
    while(radio.isSending() && millis()-started<100UL) delay(1);
    sent=!radio.isSending();
  }
  radio.setModeIdle();
  if(!sent) Serial.println(F("TX_ERROR: check radio power and wiring"));
  if(millis()-lastPrint>=250UL) {
    lastPrint=millis();
    Serial.print(F("TX POT="));Serial.print(pot);
    Serial.print(F(" ANGLE="));Serial.print(angle);
    Serial.print(F(" READY="));Serial.print(configured ? 1:0);
    for(uint8_t i=0;i<4;++i) {
      Serial.print(F(" A"));Serial.print(i);Serial.print('=');Serial.print(axes[i]);
    }
    Serial.println();
  }
  delay(40);
}
