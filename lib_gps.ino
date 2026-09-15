void initGPS() {
  //ss.begin( GPS_BAUD ) ;
  gpsSerial.begin( GPS_BAUD, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, false ) ;
}


void loopGPS() {
  if( once(1000)) {
    loopGPS1() ;
  }

  if( once(10000)) {
    loopGPS2() ;
  }
}

void loopGPS1() {
  int attempCount = 200 ;

  while( gpsSerial.available() > 0 && attempCount > 0 ) {
    char gpsData = gpsSerial.read(); 
    //Serial.print("..") ;
    //Serial.print(gpsData);

    if (gps.encode( gpsData )) {
      if (gps.location.isUpdated()) {
        Serial.print("Latitude: ") ; 
        Serial.println(gps.location.lat(), 6) ;
        Serial.print("Longitude: ") ; 
        Serial.println(gps.location.lng(), 6) ;
      }

      if( gps.satellites.isUpdated() && gps.satellites.isValid()) {
        Serial.print("GPS sattelites: ") ;
        Serial.println( gps.satellites.value() ) ;
        sendDataToBLE("GPS sattelites: " + String( gps.satellites.value()) ) ; 
      }  else {
        sendDataToBLE("! GPS: searching sattelites...") ;
        Serial.println("GPS: searching sattelites..." ) ;
      }
    }
    attempCount-- ;
  }  

  Serial.println() ;

  if( attempCount <=0 ) {
    Serial.println("! loopGPS: failed to get data from GPS") ;
  }
}

void loopGPS2() {
  if( gps.charsProcessed() < 10 ) {
    sendDataToBLE("! GPS not found") ;
    Serial.println("! No GPS detected: check wiring") ;
  }
}