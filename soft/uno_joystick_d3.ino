// VRX -> A0: bidirectional speed for a continuous-rotation servo.
// VRY -> A1: upward throttle for ESC2 on Nano D3.
// A4: external throttle potentiometer for ESC1 on Nano D2.
// Use with the matching Nano sketch (packet version 4).
#include <SPI.h>
#include <RH_NRF905.h>

// true = stick UP makes A1 smaller; false = UP makes A1 larger.
const bool UP_IS_LOW=true;
const bool REVERSE_STEERING=false;
// Keep true only if external throttle potentiometer is wired to A4.
// Set false if it is absent: D2 stays at minimum and A4 is ignored.
const bool USE_POT_MOTOR=true;
const int DEAD_ZONE=30;
const uint8_t PROTOCOL_VERSION=4;
uint16_t centerX=512,centerY=512;
bool inputOK=false;
unsigned long lastPrint=0;

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





uint16_t readInput(uint8_t pin){
  analogRead(pin); // discard previous ADC channel's first sample
  uint16_t sum=0;
  for(uint8_t i=0;i<8;++i)sum+=analogRead(pin);
  return sum/8;
}

void measureCenter(){
  uint32_t sx=0,sy=0;
  for(uint8_t i=0;i<32;++i){sx+=readInput(A0);sy+=readInput(A1);delay(2);}
  centerX=sx/32;centerY=sy/32;
  inputOK=centerX>=200 && centerX<=823 && centerY>=200 && centerY<=823;
  Serial.print(F("CENTER A0="));Serial.print(centerX);
  Serial.print(F(" A1="));Serial.println(centerY);
  if(!inputOK)Serial.println(F("BAD_CENTER: release stick/check wiring, then restart UNO"));
}

uint8_t steeringFromX(uint16_t x){
  // Encode left / stop / right as 45 / 90 / 135 in the packet.
  int command=90;
  const int leftEdge=centerX-DEAD_ZONE;
  const int rightEdge=centerX+DEAD_ZONE;
  if(x<leftEdge) command-=map(leftEdge-x,0,leftEdge,0,45);
  else if(x>rightEdge) command+=map(x-rightEdge,0,1023-rightEdge,0,45);
  if(REVERSE_STEERING) command=180-command;
  return static_cast<uint8_t>(command);
}

uint16_t throttleFromY(uint16_t y) {
  const int GAS_DEAD_ZONE = 100;

  if (UP_IS_LOW) {
    // Центр, небольшое отклонение и движение вниз — стоп.
    if (y >= centerY - GAS_DEAD_ZONE) return 0;

    return (uint16_t)map(
      y, centerY - GAS_DEAD_ZONE, 0,
      0, 1023
    );
  }

  if (y <= centerY + GAS_DEAD_ZONE) return 0;

  return (uint16_t)map(
    y, centerY + GAS_DEAD_ZONE, 1023,
    0, 1023
  );
}

void setup(){
  setupRadio();
  pinMode(A0,INPUT);pinMode(A1,INPUT);pinMode(A4,INPUT);
  Serial.println(F("Release joystick now. FIXED A0=STEERING, A1=THROTTLE"));
  delay(1000);measureCenter();
  Serial.println(F("READY: center/down=stop; up=throttle. Monitor 115200 baud"));
}

void loop(){
  // These are independent readings from two DIFFERENT physical analog pins.
  uint16_t rawX=readInput(A0);
  uint16_t rawY=readInput(A1);
  uint16_t pot=USE_POT_MOTOR?readInput(A4):0;
  uint16_t throttle=inputOK?throttleFromY(rawY):0;
  uint8_t steering=inputOK?steeringFromX(rawX):90;
  uint16_t sentPot=inputOK?pot:0;
  uint8_t packet[8]={0xC7,PROTOCOL_VERSION,static_cast<uint8_t>(inputOK?1:0),
    static_cast<uint8_t>(sentPot),static_cast<uint8_t>(sentPot>>8),steering,
    static_cast<uint8_t>(throttle),static_cast<uint8_t>(throttle>>8)};
  radio.setModeIdle();bool sent=radio.send(packet,sizeof(packet));
  if(sent){
    unsigned long started=millis();
    while(radio.isSending() && millis()-started<100UL)delay(1);
    sent=!radio.isSending();
  }
  radio.setModeIdle();
  if(!sent)Serial.println(F("TX_ERROR"));
  if(millis()-lastPrint>=250UL){
    lastPrint=millis();
    Serial.print(F("A0="));Serial.print(rawX);
    Serial.print(F(" A1="));Serial.print(rawY);
    Serial.print(F(" STEER="));Serial.print(static_cast<int>(steering)-90);
    Serial.print(F(" THR="));Serial.print(throttle);
    Serial.print(F(" POT="));Serial.print(pot);
    Serial.print(F(" INPUT_OK="));Serial.println(inputOK?1:0);
  }
  delay(40);
}
