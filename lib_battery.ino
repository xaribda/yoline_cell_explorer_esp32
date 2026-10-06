void loopBattery() {
  if( once( 6200 )) {
    gsmAsync.addCommand("AT+CBC") ;
    Serial.println(F("-request AT+CBC")) ; 
  }
}
// loopBattery

// Переменные для хранения результатов по батарейке
int chgStatus = -1;     // Статус зарядки (0-3)
int batteryPercent = -1;     // Уровень заряда в % (0-100)
int voltageMV = -1; // Напряжение в милливольтах (например, 4180)


void handleBattery(char* result) {

    int parsed = sscanf( result, "%d,%d,%d", &chgStatus, &batteryPercent, &voltageMV ) ;
      // Serial.print( "BATTERY, status" ) ; Serial.print( chgStatus ) ;
      // Serial.print( " percent" ) ; Serial.print( batteryPercent ) ;
      // Serial.print( " voltage" ) ; Serial.print( voltageMV ) ;
      // Serial.println() ;

    if (parsed == 3) {
    } else {
      Serial.print("Error while parsing the battery data: "); Serial.println( result ) ;
      chgStatus = -1 ;
      batteryPercent = -1 ;
      voltageMV = -1 ; 
    }

}
// handleBattery


String getBattery() {
  char buffer[200] ;
  snprintf( buffer, sizeof(buffer), "Status: %d percent: %d voltage: %d", chgStatus, batteryPercent, voltageMV ) ;

  return String( buffer ) ;
}