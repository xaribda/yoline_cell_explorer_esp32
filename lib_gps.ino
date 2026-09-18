double GPSLatitude = 0.0 ;
double GPSLongitude = 0.0 ;
int GPSAccuracy = 0 ;
int GPSFixType = 0 ;
int GPSSatteliteCount = 0 ;
bool weGotDataFromGPS = false ;

const uint8_t enableNmea[] = {
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x20, 0x40, 0xF5, 0x93
};

// UBX-CFG-VALSET packet: Disable UBX on UART1 (CFG-UART1OUTPROT-UBX = 0)
const uint8_t disableUbx[] = {
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x40, 0xF6, 0x9C
};

void initGPS() {
  //ss.begin( GPS_BAUD ) ;
  //gpsSerial.begin( GPS_BAUD, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, false ) ;
  gpsHardwareSerial.begin( GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN ) ;
  delay(200) ;
  configureGPS_UBX() ;

  // Serial.println("Sending: Enable NMEA packet...");
  // gpsSerial.write(enableNmea, sizeof(enableNmea));
  // delay(200); // Give the module time to process the key

  // Serial.println("Sending: Disable UBX binary packet...");
  // gpsSerial.write(disableUbx, sizeof(disableUbx));
  // delay(500);  
}


void loopGPS() {
  if( once(30)) {
    loopGPS1() ;
  }

  if( once(10000)) {
    loopGPS2() ;
  }
}

void loopGPS1() {
  int attempCount = 500 ;

  while (gpsHardwareSerial.available() > 0 && attempCount > 0 ) {
    weGotDataFromGPS = true ;
    
    // Считываем ровно один байт из буфера
    uint8_t incomingByte = gpsHardwareSerial.read();
    //Serial.print( incomingByte, HEX ) ;
    //Serial.printf("%02X ", incomingByte);

    
    // Передаем этот байт в наш парсер (стейт-машину)
    processUBXByte(incomingByte);
    attempCount-- ;
    
  }
  // Serial.println() ;
  return ;

  while( gpsHardwareSerial.available() > 0 ) {
    char gpsData = gpsHardwareSerial.read(); 
    //Serial.print("///") ;
    //Serial.print(gpsData);

    if (gps.encode( gpsData )) {
      if (gps.location.isUpdated()) {
        Serial.print("Latitude: ") ; 
        Serial.println(gps.location.lat(), 6) ;
        Serial.print("Longitude: ") ; 
        Serial.println(gps.location.lng(), 6) ;
      }

      if( gps.satellites.isValid() ) {
        String satString = String(gps.satellites.value()) ;
        String strToBLE = "GPS sattelites: " + satString ;
        sendDataToBLE( (char*) strToBLE.c_str() ) ; 
      }

      if (gps.date.isValid()) {
        Serial.print("GPS date: ");
        Serial.print(gps.date.month());
        Serial.print(F("/"));
        Serial.print(gps.date.day());
        Serial.print(F("/"));
        Serial.println(gps.date.year());
      }
      if (gps.time.isValid())
      {
        Serial.print("GPS time: ");
        if (gps.time.hour() < 10) Serial.print(F("0"));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        if (gps.time.minute() < 10) Serial.print(F("0"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        if (gps.time.second() < 10) Serial.print(F("0"));
        Serial.print(gps.time.second());
        Serial.print(F("."));
        if (gps.time.centisecond() < 10) Serial.print(F("0"));
        Serial.println(gps.time.centisecond());
      }


      if( gps.satellites.isUpdated() && gps.satellites.isValid()) {
        Serial.print("GPS sattelites: ") ;
        Serial.println( gps.satellites.value() ) ;
      }  else {
        sendDataToBLE("! GPS: searching sattelites...") ;
        Serial.println("GPS: searching sattelites..." ) ;
      }
    }
    //attempCount-- ;
  }  

  Serial.println() ;

  if( attempCount <=0 ) {
    Serial.println("! loopGPS: failed to get data from GPS") ;
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
void loopGPS2() {
  if( !weGotDataFromGPS == true ) {
    sendDataToBLE("! GPS not found (not connected)") ;
    Serial.println("! No GPS detected: check wiring") ;
  }

  if( GPSLatitude == 0.0 ) {
    sendDataToBLE("! GPS position is not fixed") ;
    Serial.println("! GPS position is not fixed") ;
  }

  if( GPSFixType > 0 ) {
    char buffer[200] ;
    snprintf( buffer, sizeof(buffer), "LL: %.7f, %.7f Sat: %d fixType: %d Acc: %d", GPSLatitude, GPSLongitude, GPSSatteliteCount, GPSFixType, GPSAccuracy ) ;
    String xd = String( buffer ) ;
    Serial.println( xd ) ;
    sendDataToBLE( buffer ) ;
    Serial.print("Lat, lng: "); Serial.print( GPSLatitude, 7 ); Serial.print( "," ) ; Serial.print( GPSLongitude, 7 ) ;
    Serial.print(" accuracy: ") ; Serial.print( GPSAccuracy ) ;
    Serial.print(" fix: "); Serial.print( GPSFixType);
    Serial.print(" satellites: "); Serial.print( GPSSatteliteCount) ;
    Serial.println() ;

  }

  if( GPSSatteliteCount > 0 ) {
    Serial.print("Sattelite count: ") ;  Serial.println( GPSSatteliteCount ) ;
  }

  weGotDataFromGPS = false ;

  return ;
  if( gps.charsProcessed() < 10 ) {
    sendDataToBLE("! GPS not found (not connected)") ;
    Serial.println("! No GPS detected: check wiring") ;
  }
}


#include <Arduino.h>

// Struct mapping the u-blox M10 NAV-PVT payload layout (Little-Endian)
// Pack structure ensures the compiler doesn't add padding bytes
struct __attribute__((packed)) UBX_NAV_PVT_Payload {
  uint32_t iTOW;
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  min;
  uint8_t  sec;
  uint8_t  valid;

  uint32_t tAcc;
  int32_t  nano;

  uint8_t  fixType;
  uint8_t  flags;
  uint8_t  flags2;
  uint8_t  numSV;

  int32_t  lon;
  int32_t  lat;
  int32_t  height;
  int32_t  hMSL;

  uint32_t hAcc;
  uint32_t vAcc;

  int32_t  velN;
  int32_t  velE;
  int32_t  velD;
  int32_t  gSpeed;
  int32_t  headMot;

  uint32_t sAcc;
  uint32_t headAcc;

  uint16_t pDOP;

  uint8_t  flags3;
  uint8_t  reserved1[5];

  int32_t  headVeh;
  int16_t  magDec;
  uint16_t magAcc;
};

static_assert(sizeof(UBX_NAV_PVT_Payload) == 92, "UBX_NAV_PVT_Payload must be 92 bytes");

// Global variables for the parser state machine
enum UBX_STATE { SYNC1, SYNC2, CLASS, ID, LENGTH_L, LENGTH_H, PAYLOAD, CHK_A, CHK_B };
UBX_STATE ubxState = SYNC1;

uint8_t msgClass, msgId;
uint16_t payloadLength;
uint16_t payloadCounter = 0;
uint8_t calcCK_A = 0, calcCK_B = 0;

// Maximum expected buffer size for standard NAV payloads
uint8_t payloadBuffer[100]; 

// Process incoming byte through state machine
void processUBXByte(uint8_t b) {
  switch (ubxState) {
    case SYNC1:
      if (b == 0xB5) ubxState = SYNC2;
      break;
      
    case SYNC2:
      if (b == 0x62) {
        ubxState = CLASS;
        calcCK_A = 0; calcCK_B = 0; // Reset checksum calculations
        payloadLength = 0; // Возможно, надо будет убрать
      } else {
        ubxState = SYNC1;
      }
      break;
      
    case CLASS:
      msgClass = b;
      calcCK_A += b; calcCK_B += calcCK_A;
      ubxState = ID;
      break;
      
    case ID:
      msgId = b;
      calcCK_A += b; calcCK_B += calcCK_A;
      ubxState = LENGTH_L;
      break;
      
    case LENGTH_L:
      payloadLength = b;
      calcCK_A += b; calcCK_B += calcCK_A;
      ubxState = LENGTH_H;
      break;
      
    case LENGTH_H:
      payloadLength |= (b << 8);
      calcCK_A += b; calcCK_B += calcCK_A;
      payloadCounter = 0;
      if (payloadLength > sizeof(payloadBuffer)) {
        ubxState = SYNC1; // Protect memory from oversized payloads
      } else {
        ubxState = (payloadLength == 0) ? CHK_A : PAYLOAD;
      }
      break;
      
    case PAYLOAD:
      payloadBuffer[payloadCounter++] = b;
      calcCK_A += b; calcCK_B += calcCK_A;
      if (payloadCounter >= payloadLength) {
        ubxState = CHK_A;
      }
      break;
      
    case CHK_A:
      if (b == calcCK_A) ubxState = CHK_B;
      else ubxState = SYNC1; // Bad Checksum
      break;
      
    case CHK_B:
      if (b == calcCK_B) {
        // Serial.println("++++++++") ;
        // SUCCESSFULLY DECODED A VALID PACKET
        if (msgClass == 0x01 && msgId == 0x07) { 
          // Cast the raw buffer into our structured layout
          UBX_NAV_PVT_Payload* pvt = (UBX_NAV_PVT_Payload*)payloadBuffer;
          
          GPSLatitude = pvt->lat / 10000000.0 ;
          GPSLongitude = pvt->lon / 10000000.0 ;
          GPSAccuracy = pvt->hAcc ;
          GPSFixType = pvt->fixType ;
          GPSSatteliteCount = pvt->numSV ;
          
          // Serial.print("Lat, lng: "); Serial.print( GPSLatitude, 10 ); Serial.print( "," ) ; Serial.print( GPSLongitude, 10 ) ;
          // Serial.print(" accuracy: ") ; Serial.print( pvt->hAcc ) ;
          // Serial.print(" fix: "); Serial.print( pvt->fixType );
          // Serial.print(" satellites: "); Serial.print( pvt->numSV ) ;
          // Serial.println() ;
        }
      }
      ubxState = SYNC1; // Reset for next message
      break;
  }
}


void configureGPS_UBX() {
  // UBX-CFG-MSG
  //
  // Class: 0x06
  // ID:    0x01
  //
  // Payload:
  // 01 = message class NAV
  // 07 = message ID PVT
  // 00 = rate on I2C
  // 01 = rate on UART1
  // 00 = rate on UART2
  // 00 = rate on USB
  // 00 = rate on SPI
  // 00 = reserved

  uint8_t msg[] = {
    0xB5, 0x62,       // UBX sync
    0x06, 0x01,       // CFG-MSG
    0x08, 0x00,       // payload length = 8

    0x01,             // msgClass = NAV
    0x07,             // msgId = PVT
    0x00,             // I2C rate
    0x01,             // UART1 rate = 1
    0x00,             // UART2 rate
    0x00,             // USB rate
    0x00,             // SPI rate
    0x00              // reserved

    // checksum будет добавлен ниже
  };

  uint8_t ckA = 0;
  uint8_t ckB = 0;

  // Checksum считается от CLASS до конца PAYLOAD
  for (int i = 2; i < sizeof(msg); i++) {
    ckA += msg[i];
    ckB += ckA;
  }

  gpsHardwareSerial.write(msg, sizeof(msg));
  gpsHardwareSerial.write(ckA);
  gpsHardwareSerial.write(ckB);

  gpsHardwareSerial.flush();

  Serial.printf(
    "GPS: UBX-NAV-PVT enabled, CK=%02X %02X\n",
    ckA,
    ckB
  );
}
