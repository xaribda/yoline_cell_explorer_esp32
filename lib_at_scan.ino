bool isAnyCellList = false ;
bool isSendCellListToBLE = false ;


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

