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

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h> // https://github.com/mikalhart/TinyGPSPlus

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
// GsmAsync

// BLE 
BLEServer* pServer = nullptr ;
BLECharacteristic* pCharacteristic = nullptr ;
bool deviceConnected = false ;
bool oldDeviceConnected = false ;

// GPS
#define GPS_TX_PIN 20
#define GPS_RX_PIN 21
#define GPS_BAUD 115200
//SoftwareSerial gpsSerial ;
SoftwareSerial modemSoftwareSerial ;
TinyGPSPlus gps ;
//SoftwareSerial ss( GPS_TX_PIN, GPS_RX_PIN ) ;


//////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setRotation(3) ;
  oled.clearDisplay() ;
  oled.setCursor(0, 40); oled.print("YOLINE");
  oled.setCursor(0, 50); oled.print("Cell");
  oled.setCursor(0, 60); oled.print("explorer");
  oled.setCursor(0, 70); oled.print("v 1.04");
  oled.display();


  Serial.begin(115200);


  delay(1000);
  Serial.println("------------------ started -------------------");

  // Настройка порта для модема (начнем с дефолтных 9600)
  //SerialAT.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN);
  modemSoftwareSerial.begin( 9600, SWSERIAL_8N1, RX_PIN, TX_PIN, false );

  //gsmAsync.init( &SerialAT, timeoutHandler, errorHandler ) ;
  gsmAsync.init( &modemSoftwareSerial, timeoutHandler, errorHandler ) ;
  gsmAsync.registerHandler( &csqHandler );
  gsmAsync.registerHandler( &handler2 );
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


  Serial.println("------------------ setup BLE --------------------") ;
  initBLE() ;

  Serial.println("------------------ setup done -------------------") ;
  oled.clearDisplay() ; 

  Serial.println("------------------ setup GPS --------------------") ;
  initGPS() ;

  Serial.println("------------------ setup CD CARD --------------------") ;
  initSD() ;
}
// setup


/////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //doTermitalImitation() ;

  if( once(2000) ) {
    Serial.println("-request AT+CENG?") ; 
    gsmAsync.addCommand("AT+CENG?") ;
  }

  gsmAsync.doLoop() ;
  loopBLE() ;

  loopGPS() ;

  loopSD() ;
}
// loop









void handleCsq(char* result) {
  int rssi;
  sscanf(result, "%d", &rssi);
  Serial.print("Signal quality:");
  Serial.println(rssi);
}

void handleCellList(char* result) {
  sendDataToBLE( result ) ;
  displayOneCell( result ) ;
  Serial.println( result ) ;
}
// handleCellList


void timeoutHandler() {
  oled.setCursor(0, 0);
  oled.print("! GSM timeout");
  oled.display();

  sendDataToBLE("! GSM timeout") ;
  Serial.println(F("GSM not responding"));
}
// timeoutHandler


void errorHandler() {
  oled.setCursor(0, 0);
  oled.print("! GSM Error");
  oled.display();

  sendDataToBLE("! GSM error") ;
  Serial.println(F("GSM Error"));
}
// errorHandler







