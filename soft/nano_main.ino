#include <SPI.h>
#include <RH_NRF905.h>
#include <avr/interrupt.h>

// =====================================================
// nRF905
// =====================================================
// CE    -> D8
// TX_EN -> D9
// CSN   -> D10
// MOSI  -> D11
// MISO  -> D12
// SCK   -> D13

RH_NRF905 radio(8, 9, 10);

// =====================================================
// ESC на D2
// =====================================================

const byte ESC_PIN = 2;

// Timer2 работает с шагом 8 мкс.
// 1000 мкс / 8 = 125
volatile uint8_t escPulseTicks = 125;

uint8_t frameCounter = 0;

// Последняя команда
uint16_t potValue = 0;

uint16_t targetPulse = 1000;
uint16_t currentPulse = 1000;

bool armed = false;

unsigned long lastSmooth = 0;
unsigned long lastPrint = 0;


// =====================================================
// Установка импульса ESC
// =====================================================

void setEscPulse(uint16_t us)
{
  us = constrain(us, 1000, 1600);

  // Однобайтовая запись -> безопасна относительно ISR
  escPulseTicks = us / 8;
}


// =====================================================
// Timer2 overflow
//
// prescaler = 128
// 16 MHz / 128 = 125 kHz
// один tick = 8 мкс
//
// 256 ticks = 2048 мкс
//
// 10 overflow ~= 20.48 мс
// =====================================================

ISR(TIMER2_OVF_vect)
{
  frameCounter++;

  if (frameCounter >= 10)
  {
    frameCounter = 0;

    // D2 HIGH
    PORTD |= _BV(PD2);

    // Через заданное время выключим импульс
    OCR2A = escPulseTicks;

    // Сбрасываем старый флаг compare
    TIFR2 |= _BV(OCF2A);

    // Разрешаем compare interrupt
    TIMSK2 |= _BV(OCIE2A);
  }
}


// =====================================================
// Конец импульса ESC
// =====================================================

ISR(TIMER2_COMPA_vect)
{
  // D2 LOW
  PORTD &= ~_BV(PD2);

  // Больше compare interrupt до следующего импульса
  TIMSK2 &= ~_BV(OCIE2A);
}


// =====================================================
// Запуск Timer2
// =====================================================

void setupEscTimer()
{
  pinMode(ESC_PIN, OUTPUT);
  digitalWrite(ESC_PIN, LOW);

  cli();

  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // Сброс флагов
  TIFR2 = _BV(TOV2) | _BV(OCF2A);

  // Разрешаем overflow interrupt
  TIMSK2 = _BV(TOIE2);

  // Timer2 prescaler = 128
  TCCR2B = _BV(CS22) | _BV(CS20);

  sei();
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  Serial.println("Запуск ESC...");

  // Стабильные 1000 мкс начинает генерировать Timer2
  setEscPulse(1000);
  setupEscTimer();

  // 5 секунд ESC получает минимальный газ
  delay(5000);

  Serial.println("ESC готов");

  // ===================================================
  // nRF905
  // ===================================================

  Serial.println("Запуск nRF905...");

  if (!radio.init())
  {
    Serial.println("ERROR: nRF905 не инициализировался");

    // При ошибке радио ESC остаётся на стопе
    setEscPulse(1000);

    while (1);
  }

  radio.setChannel(108);

  Serial.println("nRF905 готов");
  Serial.println("Поставь потенциометр в минимум");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ===================================================
  // Получение радио
  // ===================================================

  if (radio.available())
  {
    uint16_t receivedValue = 0;
    uint8_t len = sizeof(receivedValue);

    if (radio.recv((uint8_t*)&receivedValue, &len))
    {
      if (len == sizeof(receivedValue))
      {
        potValue = constrain(receivedValue, 0, 1023);

        // ===============================================
        // ARMING
        // ===============================================

        if (!armed)
        {
          targetPulse = 1000;

          if (potValue <= 50)
          {
            armed = true;

            Serial.println("ARMED!");
          }
        }

        // ===============================================
        // Новая скорость
        // ===============================================

        if (armed)
        {
          if (potValue <= 30)
          {
            targetPulse = 1000;
          }
          else
          {
            targetPulse = map(
              potValue,
              0, 1023,
              1000, 1600
            );

            targetPulse = constrain(
              targetPulse,
              1000, 1600
            );
          }
        }
      }
    }
  }


  // ===================================================
  // Плавное изменение скорости
  // ===================================================
  //
  // Вместо моментального:
  //
  // 1100 -> 1500
  //
  // делаем:
  //
  // 1100
  // 1110
  // 1120
  // ...
  // 1500
  //
  // Это дополнительно уменьшает рывки.
  // ===================================================

  if (millis() - lastSmooth >= 10)
  {
    lastSmooth = millis();

    if (currentPulse < targetPulse)
    {
      currentPulse += 5;

      if (currentPulse > targetPulse)
        currentPulse = targetPulse;
    }

    else if (currentPulse > targetPulse)
    {
      currentPulse -= 5;

      if (currentPulse < targetPulse)
        currentPulse = targetPulse;
    }

    setEscPulse(currentPulse);
  }


  // ===================================================
  // Serial
  // ===================================================

  if (millis() - lastPrint >= 250)
  {
    lastPrint = millis();

    Serial.print("POT=");
    Serial.print(potValue);

    Serial.print(" TARGET=");
    Serial.print(targetPulse);

    Serial.print(" ESC=");
    Serial.print(currentPulse);

    Serial.print(" ARMED=");
    Serial.println(armed ? "YES" : "NO");
  }
}