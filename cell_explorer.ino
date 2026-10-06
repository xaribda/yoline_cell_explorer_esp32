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

#include <SoftwareSerial.h>
//#include <TinyGPSPlus.h> // https://github.com/mikalhart/TinyGPSPlus

#include "once.h"




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
void timeoutHandler() ;
void errorHandler() ;
void handleCsq(char* result) ;
void handleCellList(char* result) ;

GsmHandler csqHandler = { "+CSQ:", handleCsq };
GsmHandler handler2 = { "+CENG:", handleCellList };
void handleBattery( char* result ) ;
GsmHandler batteryHandler = { "+CBC:", handleBattery };
// GsmAsync

// GPS
#define GPS_TX_PIN 20
#define GPS_RX_PIN 21
#define GPS_BAUD 115200
//SoftwareSerial gpsSerial ;
SoftwareSerial modemSoftwareSerial ;
//TinyGPSPlus gps ;
//SoftwareSerial ss( GPS_TX_PIN, GPS_RX_PIN ) ;

uint64_t chipMacAddress ;
char chipMacAddressBuffer[13]; // 12 + символ конца строки '\0'
String formattedChiMacAddress = "" ;
String globalDeviceName = "" ;
String globalDeviceVersion = "1.80" ;



//////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200) ;
  delay(1000) ;
  Serial.println("------------------ YOLINE Cell Explorer (c) 2026 -------------------");
  Serial.println("------------------ started -----------------------------------------");

  chipMacAddress = ESP.getEfuseMac() ;
  snprintf( chipMacAddressBuffer, sizeof(chipMacAddressBuffer), "%04X%08X", (uint16_t)( chipMacAddress >> 32), (uint32_t) chipMacAddress ) ;
  Serial.print("Serial number: ") ;
  char formattedMacChar[18] ; 
  sprintf( formattedMacChar, "%.2s-%.2s-%.2s-%.2s-%.2s-%.2s", &chipMacAddressBuffer[0], &chipMacAddressBuffer[2], &chipMacAddressBuffer[4], &chipMacAddressBuffer[6], &chipMacAddressBuffer[8], &chipMacAddressBuffer[10]) ;
  formattedChiMacAddress = String(formattedMacChar)   ;
  globalDeviceName = "YOLINE RF " + formattedChiMacAddress ;

  Serial.println( formattedChiMacAddress ) ;

  oled.begin( SSD1306_SWITCHCAPVCC, 0x3C) ;
  oled.setTextSize( 1 ) ;
  oled.setTextColor( WHITE, BLACK ) ; 
  oled.setRotation(1) ;
  oled.clearDisplay() ;
  oled.setCursor(0, 30); oled.print( "YOLINE");
  oled.setCursor(0, 40); oled.print( "RF Fly");
  oled.setCursor(0, 50); oled.print( "Scanner");                                      
  oled.setCursor(0, 60); oled.print( "v "); oled.print( getDeviceVersion() ) ;
  //oled.drawLine( 0, 80, 34, 80, WHITE ) ;
  drawDottedHLine( 0, 80, 34, WHITE ) ;
  oled.setCursor(0, 85); oled.write( (uint8_t*) chipMacAddressBuffer, 6 ) ; 
  oled.setCursor(0, 95); oled.write( chipMacAddressBuffer + 6 ) ; 

  oled.display();


  // Настройка порта для модема (начнем с дефолтных 9600)
  //SerialAT.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN);
  modemSoftwareSerial.begin( 9600, SWSERIAL_8N1, RX_PIN, TX_PIN, false );

  //gsmAsync.init( &SerialAT, timeoutHandler, errorHandler ) ;
  gsmAsync.init( &modemSoftwareSerial, timeoutHandler, errorHandler ) ;
  gsmAsync.registerHandler( &csqHandler );
  gsmAsync.registerHandler( &handler2 );
  gsmAsync.registerHandler( &batteryHandler );
  gsmAsync.addCommand("ATE0" ) ; // Выключить эхо команд
  gsmAsync.addCommand("AT+CSQ") ; // Уровень сигнала
  delay( 1000 ) ; 

  // Serial.println("-- speed up to 115200--") ;
  // gsmAsync.addCommand("AT+IPR=19300" ) ; // Поднимаем скорость
  // delay(100); 
  // SerialAT.end(); // Закрываем соединение на 9600
  // delay(100); 
  // SerialAT.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Открываем на 115200
  // delay(500);
  // Serial.println("-- speed completed--") ;

  gsmAsync.addCommand("AT+CENG=1,1") ; // Инженерный режим с выдачей cellid
  gsmAsync.addCommand("AT+CENG?") ; // Список сот
  gsmAsync.addCommand("AT+CSQ") ; // Еще раз


  Serial.println("------------------ setup BLE ---------------------------------------") ;
  initBLE() ;
  oled.clearDisplay() ; 

  Serial.println("------------------ setup GPS ---------------------------------------") ;
  initGPS() ;

  Serial.println("------------------ setup CD CARD -----------------------------------") ;
  initSD() ;

  drawStatusPlacement() ;

  Serial.println("------------------ setup WEB server --------------------------------") ;
  initWEB() ;  
  //startWebServer(13) ;
}
// setup


/////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //doTermitalImitation() ;

  if( once(2000) ) {
    requestCellList() ;
  }

  gsmAsync.doLoop() ;
  loopBLE() ;

  loopGPS() ;

  loopSD() ;

  loopBattery() ;

  loopStatusBar() ;

  loopWEB() ;
}
// loop








