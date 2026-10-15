bool isAnyCellList = false ;
bool isSendCellListToBLE = false ;



#define MODEM_EXCHANGE_SPEED 9600

////////////////////////////////////////////////////////////////////////////////////////////////
// Последовательность шагов при инициализации модема
#define MODEM_INIT_STEP_1 1 // AT -> OK
#define MODEM_INIT_STEP_2 2 // AT+CPIN? -> READY
#define MODEM_INIT_STEP_3 3 // AT+CREG? -> ???
#define MODEM_INIT_STEP_4 4 // AT+CENG? -> список сот
int initModeSate = MODEM_INIT_STEP_1 ;

////////////////////////////////////////////////////////////////////////////////////////////////
// Обработчики AT команд
void handleOkResponse(char* result) ;
void timeoutHandler() ;
void errorHandler() ;
void handleCsq(char* result) ;
void handleCellList(char* result) ;
void handleBattery( char* result ) ;

// GsmHandler csqHandler = { "+CSQ:", handleCsq } ;
// GsmHandler cellListHandler = { "+CENG:", handleCellList } ;
// GsmHandler handlerCPIN = { "+CPIN:", handleCPIN } ;
// GsmHandler batteryHandler = { "+CBC:", handleBattery };
// GsmAsync


////////////////////////////////////////////////////////////////////////////////////////////////
void initAT() {
  // Настройка порта для модема (начнем с дефолтных 9600, потом поднимаем)
  //SerialAT.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN);
  //modemSoftwareSerial.begin( 9600, SWSERIAL_8N1, RX_PIN, TX_PIN, false );
  Serial0.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN ) ;
  //Serial0.println( "AT+IPR=9600" ) ; 
  Serial0.println( "AT+IPR=" + String( MODEM_EXCHANGE_SPEED )) ; 
  delay( 200 ) ;
  Serial0.end() ; 
  delay( 100 ) ;
  Serial0.begin( MODEM_EXCHANGE_SPEED, SERIAL_8N1, RX_PIN, TX_PIN ) ; 

  //gsmAsync.init( &SerialAT, timeoutHandler, errorHandler ) ;
  // gsmAsync.init( &modemSoftwareSerial, timeoutHandler, errorHandler ) ;
  gsmAsync.init( &Serial0, timeoutHandler, errorHandler ) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CSQ:", handleCsq }) ) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CENG:", handleCellList }) ) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CBC:", handleBattery }) ) ;
  gsmAsync.registerHandler( new GsmHandler({ "OK", handleOkResponse })) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CPIN:", handleCPIN })) ;
  gsmAsync.registerHandler( new GsmHandler({ "SIM800", handleOkResponse })) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CREG:", handleCREGResponse })) ;
  gsmAsync.addCommand( "ATE0" ) ; // Выключить эхо команд
  //gsmAsync.addCommand("AT+CSQ") ; // Уровень сигнала
  delay( 1000 ) ; 

  startInitModemSequence() ;

}
// initAT


void rebootModem() {
  gsmAsync.addCommand("AT+CFUN=1,1") ;
  startInitModemSequence() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////
void startInitModemSequence() {
  initModeSate = MODEM_INIT_STEP_3 ;
}
// startInitModemSequence


////////////////////////////////////////////////////////////////////////////////////////////////
void loopAT() {
  if( once( 3000 )) {
    if( initModeSate == MODEM_INIT_STEP_1 ) {
      Serial.println( F("*** Modem #1")) ;
      gsmAsync.addCommand("ATI") ;
    }

    if( initModeSate == MODEM_INIT_STEP_2 ) {
      Serial.println( F("*** Modem #2")) ;
      gsmAsync.addCommand("AT+CPIN?") ;
    }

    if( initModeSate == MODEM_INIT_STEP_3 ) {
      Serial.println( F("*** Modem #3")) ;
      gsmAsync.addCommand("AT+CREG?") ;
      gsmAsync.addCommand("AT+CENG=1,1") ; // Инженерный режим с выдачей cellid
    }

    if( initModeSate == MODEM_INIT_STEP_4 ) {
      Serial.println( F("*** Modem #4")) ;
      requestCellList() ;
    }
  }
}
// loopAT


////////////////////////////////////////////////////////////////////////////////////////////////
void requestCellList() {
    // Serial.println("-request AT+CENG?") ; 
    gsmAsync.addCommand("AT+CENG?") ;
    // Сначала пишем GPS координаты и дату-время, функция не работает, если нет фикса
    toSDBuffer( getGNSSDataAndDateTimeString() ) ; toSDBuffer( "\n" ) ; 
}
// requestCellList


////////////////////////////////////////////////////////////////////////////////////////////////
void handleOkResponse(char* result) {
  Serial.println( F("*** Modem AT->OK")) ;
  initModeSate = MODEM_INIT_STEP_2 ;
}
// handleOkResponse


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCREGResponse(char* result) {
  Serial.print( F("*** Modem CREG->")) ; Serial.println( result ) ;
  // Ищем последнюю цифру в строке ответа
  int status = -1;
  char* p = strrchr( result, ',' ) ; // Ищем запятую (для формата <n>,<stat>)
  if( p != NULL ) {
      status = atoi( p + 1 ) ;
  } else {
      status = atoi( result) ; // Запятой нет, значит пришло чистое число (URC формат)
  }

  Serial.print("ok, status: ") ; Serial.println( status ) ;

  // Регистрация в домашней сети или роуминге
  if( status == 1 || status == 5 ) {
    initModeSate = MODEM_INIT_STEP_4 ;
  }
}
// handleCREGResponse




////////////////////////////////////////////////////////////////////////////////////////////////
void handleCPIN(char* result) {
  Serial.print( F("*** Modem CPIN->READY : ")) ; Serial.println( result ) ;
  initModeSate = MODEM_INIT_STEP_3 ;
}
// handleCPIN


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCsq(char* result) {
  int rssi;
  sscanf(result, "%d", &rssi);
  Serial.print(F("Signal quality:"));
  Serial.println(rssi);
}


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCellList(char* result) {
  if( isSendCellListToBLE ) { 
    sendDataToBLE( result ) ;
    Serial.println( result ) ;
  }

  // "1,1" не пишем
  if( result[0] == '1' && result[1] == ',' && result[2] == '1' ) return ;
  

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

bool isGSM() {
  return isAnyCellList ;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Модем инициализирован и зарегистрирован в сети
bool isModemRegistered() {
  return initModeSate == MODEM_INIT_STEP_4 ;
}
// isModemRegistered
