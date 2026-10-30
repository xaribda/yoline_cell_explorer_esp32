#include <Arduino.h>
#include "esp32-hal-rmt.h"

#define LED_PIN 2
#define LED_ERROR_TIME_MS 100 
#define LED_SHORT_TIME_MS 30 

static rmt_obj_t *ledRmt = nullptr;
static rmt_data_t ledData[25];

long int ledTimeMS = 0 ;

//////////////////////////////////////////////////////////////////////////////////////
void initLED() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Инициализация RMT: передача, 64 символа памяти
    ledRmt = rmtInit(LED_PIN, true, RMT_MEM_64);

    if (ledRmt == nullptr) {
        return;
    } 

    // Устанавливаем тик около 100 нс
    rmtSetTick(ledRmt, 100.0);
}
// initLED


//////////////////////////////////////////////////////////////////////////////////////
void loopLED() {
    if( ledTimeMS > 0 && ledTimeMS < millis() ) {
      ledTimeMS = 0 ;
      led( 0, 0, 0 ) ;  
    } 

    if( once( 1000 )) {
        // Нет GSM модуля
        if( !isGSMModemConnected() ) {
            Serial.println("BLINK:: isGSMModemConnected") ;
            blink( 55, 0, 0 ) ;
            return ;            
        }

        // GSM модуль слишком долго отвечает
        if ( isGSMTimeout() ) {
            Serial.println("BLINK:: isGSMTimeout") ;
            blink( 55, 0, 0 ) ;
            return ;            
        }

        // Нет списка сот
        if ( !isGSM() ) {
            Serial.println("BLINK:: isGSM") ;
            blink( 55, 0, 0 ) ;
            return ;            
        }

        // SD карта готова к записи
        if( !isSDReady() ) {
            Serial.println("BLINK:: isSDReady") ;
            blink( 55, 55, 0 ) ;
            return ;            
        }

        // Нет данных от GNSS модуля
        if( isBadGNSSConnection() ) {
            Serial.println("BLINK:: isBadGNSSConnection") ;
            blink( 0, 55, 0 ) ;
            return ;            
        }
            

        // Нет положения
        if( isBadFix() ) {
            Serial.println("BLINK:: isBadFix") ;
            blink( 15, 75, 0 ) ;
            return ;            
        }

        // Все в порядке, готовы к записи - редко мигаем белым
        if( once( 3000 ) && !isRecording() ) {
            Serial.println("BLINK:: Ready!") ;
            shortBlink( 70, 100, 100 ) ;
        }

        if( isRecording() ) {
            led( 0, 0, 20 ) ;
        }

    }

}
// loopLED


void led(uint8_t r, uint8_t g, uint8_t b) {
    if (ledRmt == nullptr) {
        return;
    }

    uint8_t color[3] = {g, r, b}; // Порядок GRB верный для WS2812
    int i = 0;

    for (int c = 0; c < 3; c++) {
        for (int bit = 7; bit >= 0; bit--) {
            if (color[c] & (1 << bit)) {
                // Бит 1
                ledData[i].level0 = 1;
                ledData[i].duration0 = 8;
                ledData[i].level1 = 0;
                ledData[i].duration1 = 4;
            } else {
                // Бит 0
                ledData[i].level0 = 1;
                ledData[i].duration0 = 4;
                ledData[i].level1 = 0;
                ledData[i].duration1 = 8;
            }
            i++;
        }
    }

    // 2. ДОБАВЛЯЕМ СТОП-БИТ (Маркер конца данных для RMT)
    ledData[24].level0 = 0;
    ledData[24].duration0 = 0;
    ledData[24].level1 = 0;
    ledData[24].duration1 = 0;

    // 3. Отправляем все 25 элементов
    rmtWrite(ledRmt, ledData, 25);

    // Пауза для фиксации цвета (reset)
    delayMicroseconds(100);
}
// led


///////////////////////////////////////////////////////////////////////////////////
void led2( uint8_t color ) {
    led( color, color, color ) ;
}
// led2


///////////////////////////////////////////////////////////////////////////////////
void blink( uint8_t r, uint8_t g, uint8_t b ) {
    led( r, g, b ) ;
    startLEDcountdown() ;
}
// blink


///////////////////////////////////////////////////////////////////////////////////
void startLEDcountdown() {
  ledTimeMS = millis() + LED_ERROR_TIME_MS ;
}
// startLEDcountdown


void shortBlink( uint8_t r, uint8_t g, uint8_t b ) {
  led( r, g, b ) ;
  ledTimeMS = millis() + LED_SHORT_TIME_MS ;
}
