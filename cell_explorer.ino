#include <HardwareSerial.h>
#include <GsmAsync.h>
#include "Adafruit_SSD1306.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FS.h> 
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

//#include <SoftwareSerial.h>
//#include <TinyGPSPlus.h> // https://github.com/mikalhart/TinyGPSPlus

#include "once.h"


// Таймпут watch dog
#define WDT_TIMEOUT 3



Adafruit_SSD1306 oled( 128, 64, &Wire, -1 ) ;

// AT+CENG=1,1  # переход в режим с выдачей cellid
// AT+CENG?     # запросить список сот
// 

// Назначаем новые пины
#define RX_PIN 3  // Сюда подключаем 3VT (TX модема)
#define TX_PIN 4  // Сюда подключаем 3VR (RX модема)


// Создаем аппаратный сериал для модема
//HardwareSerial SerialAT(1);
HardwareSerial gpsHardwareSerial( 1 ) ;

// Асинхронная обработка AT команд (https://github.com/prampec/GsmAsync)
GsmAsync gsmAsync ;

// GPS
#define GPS_TX_PIN 20
#define GPS_RX_PIN 21
#define GPS_BAUD 115200
//SoftwareSerial gpsSerial ;
//SoftwareSerial modemSoftwareSerial ;
//TinyGPSPlus gps ;
//SoftwareSerial ss( GPS_TX_PIN, GPS_RX_PIN ) ;

uint64_t chipMacAddress ;
char chipMacAddressBuffer[13]; // 12 + символ конца строки '\0'
String formattedChiMacAddress = "" ;
String globalDeviceName = "" ;
String globalDeviceVersion = "1.98" ;



//////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  initLED() ;
  led2( 1 ) ;

  Serial.begin( 115200 ) ;
  for( uint8_t i = 0 ; i < 55 ; i++ ) {
    led2( i ) ;
    delay( 10 ) ;
  }

  Serial.println(F("------------------ YOLINE Cell Explorer (c) 2026 -------------------"));
  Serial.println(F("------------------ started -----------------------------------------"));


  chipMacAddress = ESP.getEfuseMac() ;
  snprintf( chipMacAddressBuffer, sizeof(chipMacAddressBuffer), "%04X%08X", (uint16_t)( chipMacAddress >> 32), (uint32_t) chipMacAddress ) ;
  Serial.print(F("Serial number: ")) ;
  char formattedMacChar[18] ; 
  sprintf( formattedMacChar, "%.2s-%.2s-%.2s-%.2s-%.2s-%.2s", &chipMacAddressBuffer[0], &chipMacAddressBuffer[2], &chipMacAddressBuffer[4], &chipMacAddressBuffer[6], &chipMacAddressBuffer[8], &chipMacAddressBuffer[10]) ;
  formattedChiMacAddress = String(formattedMacChar)   ;
  globalDeviceName = "YOLINE RF " + formattedChiMacAddress ;

  led2( 55 ) ;


  Serial.println( formattedChiMacAddress ) ;

  oled.begin( SSD1306_SWITCHCAPVCC, 0x3C) ;
  oled.setTextSize( 1 ) ;
  oled.setTextColor( WHITE, BLACK ) ; 
  oled.setRotation(1) ;
  oled.clearDisplay() ;
  oled.setCursor(0, 30); oled.print( F("YOLINE")) ;
  oled.setCursor(0, 40); oled.print( F("RF Fly")) ;
  oled.setCursor(0, 50); oled.print( F("Scanner")) ;                                      
  oled.setCursor(0, 60); oled.print( F("v ")); oled.print( getDeviceVersion() ) ;
  //oled.drawLine( 0, 80, 34, 80, WHITE ) ;
  drawDottedHLine( 0, 80, 14, WHITE ) ;
  oled.setCursor(0, 85); oled.write( (uint8_t*) chipMacAddressBuffer, 6 ) ; 
  oled.setCursor(0, 95); oled.write( chipMacAddressBuffer + 6 ) ; 

  oled.display();

  led2( 75 ) ;

  Serial.println(F("------------------ setup GSM ---------------------------------------")) ;
  initAT() ;

  led2( 95 ) ;


  Serial.println(F("------------------ setup BLE ---------------------------------------")) ;
  initBLE() ;
  oled.clearDisplay() ; 

  led2( 115 ) ;

  Serial.println(F("------------------ setup GPS ---------------------------------------")) ;
  initGPS() ;

  led2( 125 ) ;

  Serial.println(F("------------------ setup CD CARD -----------------------------------")) ;
  initSD() ;

  led2( 145 ) ;

  drawStatusPlacement() ;

  Serial.println(F("------------------ setup WEB server --------------------------------")) ;
  initWEB() ;  
  //startWebServer(13) ;

  led2( 165 ) ;

  oled.clearDisplay() ;
  drawDottedHLine( 0, 100, 63, WHITE ) ;
  oled.setCursor(0, 105); oled.print( F("Init done!")) ;
  oled.display();

  esp_task_wdt_init( WDT_TIMEOUT, true); 
  esp_task_wdt_add( NULL); 

  led2( 175 ) ;

}
// setup


/////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  loopLED() ;

  loopAT() ;

  gsmAsync.doLoop() ;

  loopBLE() ;

  loopGPS() ;

  loopSD() ;

  loopBattery() ;

  loopStatusBar() ;

  loopWEB() ;

  if( once(6000)) {
    Serial.printf( "--- Free Heap: %d bytes\n", ESP.getFreeHeap());
  //   Serial.printf("--- Max Alloc Block: %d bytes\n", ESP.getMaxAllocHeap());
  //   Serial.printf("--- Min Free Heap Ever: %d bytes\n", ESP.getMinFreeHeap());    
  }

  delay(1) ;
  esp_task_wdt_reset() ; 
}
// loop






