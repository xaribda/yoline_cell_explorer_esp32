bool isAnyCellList = false ;
bool isSendCellListToBLE = false ;


////////////////////////////////////////////////////////////////////////////////////////////////
void initAT() {
  // Настройка порта для модема (начнем с дефолтных 9600, потом поднимаем)
  //SerialAT.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN);
  //modemSoftwareSerial.begin( 9600, SWSERIAL_8N1, RX_PIN, TX_PIN, false );
  Serial0.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN ) ;
  Serial0.println( "AT+IPR=115200" ) ; 
  delay( 200 ) ;
  Serial0.end() ; 
  delay( 100 ) ;
  Serial0.begin( 115200, SERIAL_8N1, RX_PIN, TX_PIN ) ; 

  //gsmAsync.init( &SerialAT, timeoutHandler, errorHandler ) ;
  // gsmAsync.init( &modemSoftwareSerial, timeoutHandler, errorHandler ) ;
  gsmAsync.init( &Serial0, timeoutHandler, errorHandler ) ;
  gsmAsync.registerHandler( &csqHandler );
  gsmAsync.registerHandler( &handler2 );
  gsmAsync.registerHandler( &batteryHandler );
  gsmAsync.addCommand("ATE0" ) ; // Выключить эхо команд
  gsmAsync.addCommand("AT+CSQ") ; // Уровень сигнала
  delay( 1000 ) ; 

}
// initAT


////////////////////////////////////////////////////////////////////////////////////////////////
void requestCellList() {
    // Serial.println("-request AT+CENG?") ; 
    gsmAsync.addCommand("AT+CENG?") ;
    // Сначала пишем GPS координаты и дату-время, функция не работает, если нет фикса
    toSDBuffer( getGNSSDataAndDateTimeString() ) ; toSDBuffer( "\n" ) ; 
}
// requestCellList


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCsq(char* result) {
  int rssi;
  sscanf(result, "%d", &rssi);
  Serial.print("Signal quality:");
  Serial.println(rssi);
}


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCellList(char* result) {
  if( isSendCellListToBLE ) { 
    sendDataToBLE( result ) ;
    Serial.println( result ) ;
  }


  // Если список сот, значит GSM модуль нашел сеть
  if( result[0] == '1' && result[1] == ',' ) {
    isAnyCellList = true ;
  }

  toSDBuffer( result ) ; toSDBuffer( "\n" ) ; 
  displayOneCell( result ) ;
}
// handleCellList


////////////////////////////////////////////////////////////////////////////////////////////////
void timeoutHandler() {
  oled.setCursor(0, 20);
  oled.print("GSM err 01");
  oled.display();

  sendDataToBLE("! GSM timeout") ;
  Serial.println(F("GSM not responding"));
}
// timeoutHandler


////////////////////////////////////////////////////////////////////////////////////////////////
void errorHandler() {
  oled.setCursor(0, 40);
  oled.print("GSM err 02");
  oled.display();

  sendDataToBLE("! GSM error") ;
  Serial.println(F("GSM Error"));
}
// errorHandler


////////////////////////////////////////////////////////////////////////////////////////////////
String getDeviceName() {
  return globalDeviceName ;
}
// getDeviceName


////////////////////////////////////////////////////////////////////////////////////////////////
String getSerialFormatted() {
  return formattedChiMacAddress ;
}
// getSerialFormatted


////////////////////////////////////////////////////////////////////////////////////////////////
String getDeviceVersion() {
  return globalDeviceVersion ;
}
// getDeviceVersion

bool isGPS() {
  return isAnyCellList ;
}

