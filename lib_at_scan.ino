
// Назначаем пины
#define RX_PIN 3  // Сюда подключаем 3VT (TX модема)
#define TX_PIN 4  // Сюда подключаем 3VR (RX модема)


//#define MODEM_EXCHANGE_SPEED 9600
#define MODEM_EXCHANGE_SPEED 115200

////////////////////////////////////////////////////////////////////////////////////////////////
// Последовательность шагов при инициализации модема
#define MODEM_INIT_STEP_1 1 // AT -> OK
#define MODEM_INIT_STEP_2 2 // AT+CPIN? -> READY
#define MODEM_INIT_STEP_3 3 // AT+CREG? -> ???
#define MODEM_INIT_STEP_4 4 // AT+CENG? -> список сот
int initModeSate = MODEM_INIT_STEP_1 ;

bool GSMModemConnected = false ;
bool GSMTimeout = true ;
bool isAnyCellList = false ;
bool isSendCellListToBLE = false ;


#define MAX_GSM_ERROR_COUNT 4
int GSMErrorCount = 0 ;

#define MAX_GSM_REGISTRATION_FAIL_COUNT 5
int GSMRegistrationFailedCount = 0 ;


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
  Serial0.end() ;
  delay( 100 ) ;
  Serial0.begin( 9600, SERIAL_8N1, RX_PIN, TX_PIN ) ;
  delay( 200 ) ;
  Serial0.println( "AT+IPR=9600" ) ; 
  esp_task_wdt_reset() ; 
  delay( 200 ) ;

  //Serial0.println( "AT+IPR=9600" ) ; 
  // Serial0.println( "AT+IPR=" + String( MODEM_EXCHANGE_SPEED )) ; 
  // Serial0.end() ; 
  // delay( 100 ) ;
  // Serial0.begin( MODEM_EXCHANGE_SPEED, SERIAL_8N1, RX_PIN, TX_PIN ) ; 

  GSMErrorCount = 0 ;   
  GSMRegistrationFailedCount = 0 ;

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
  gsmAsync.registerHandler( new GsmHandler({ "+CME ERROR:", handleCmeError })) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CMS ERROR:", handleCmsError })) ;
  gsmAsync.registerHandler( new GsmHandler({ "+CCID:", handleCCID })) ;
  gsmAsync.addCommand( "ATE0" ) ; // Выключить эхо команд
  //gsmAsync.addCommand("AT+CSQ") ; // Уровень сигнала
  esp_task_wdt_reset() ;
  delay( 900 ) ; 

  startInitModemSequence() ;

}
// initAT


void rebootModem() {
  gsmAsync.addCommand("AT+CFUN=1,1") ;
  startInitModemSequence() ;
  GSMModemConnected = false ;
}


////////////////////////////////////////////////////////////////////////////////////////////////
void restartGSMRegistration() {
  // Дадим себе запас по времени в 5 секунд
  esp_task_wdt_init( 5, true ) ; 

  // Все возвращаем в исходное состояние
  GSMErrorCount = 0 ;
  GSMRegistrationFailedCount = 0 ; 
  initModeSate == MODEM_INIT_STEP_1 ;

  gsmAsync.addCommand("AT+COPS=0") ;
  delay(1000) ;

  gsmAsync.addCommand("AT+CFUN=0") ;
  delay(1000) ;
  
  gsmAsync.addCommand("AT+CFUN=1") ;
  Serial.println(F("------ Forced GSM registration")) ;
 
  // Все возвращаем в исходное состояние
  esp_task_wdt_init( WDT_TIMEOUT, true ) ; 
}
// restartGSMRegistration


////////////////////////////////////////////////////////////////////////////////////////////////
void startInitModemSequence() {
  initModeSate = MODEM_INIT_STEP_1 ;
}
// startInitModemSequence


////////////////////////////////////////////////////////////////////////////////////////////////
void loopAT() {
  gsmAsync.doLoop() ;

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
    modemIsOk() ;
}
// requestCellList


////////////////////////////////////////////////////////////////////////////////////////////////
void handleOkResponse(char* result) {
  Serial.println( F("*** Modem AT->OK")) ;
  initModeSate = MODEM_INIT_STEP_2 ;
  modemIsOk() ;
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


  // Нулевой статус означает, что модем перестал пытаться зарегистрироваться в сети, даем ему несколько попыток и заставляем регистрироваться по-новой
  if( status == 0 ) {
    GSMRegistrationFailedCount++ ;

    if( GSMRegistrationFailedCount > MAX_GSM_REGISTRATION_FAIL_COUNT ) {
      sendDataToBLE("GSM: restart network search..") ;
      return restartGSMRegistration() ;
    }
  }

  // Идет поиск сети
  if( status == 2 ) {
    sendDataToBLE("GSM: searching for network..") ;
  }

  // Регистрация в домашней сети или роуминге
  if( status == 1 || status == 5 ) {
    initModeSate = MODEM_INIT_STEP_4 ;
    GSMRegistrationFailedCount = 0 ;
  } else {
    sendDataToBLE("GSM: this SIM card is not alllowed to register") ;
  }
  modemIsOk() ;
}
// handleCREGResponse




////////////////////////////////////////////////////////////////////////////////////////////////
void handleCPIN(char* result) {
  Serial.print( F("*** Modem AT+CPIN: ")) ; Serial.println( result ) ;

  if( strcmp( result, "READY" ) == 0 ){
    initModeSate = MODEM_INIT_STEP_3 ;
    modemIsOk() ;
  } else {
    gsmAsync.addCommand("AT+CCID?") ;
  }
}
// handleCPIN


////////////////////////////////////////////////////////////////////////////////////////////////
// ID сим карты
void handleCCID( char* result) {
  Serial.print( F("*** Modem AT+CCID: ")) ; Serial.println( result ) ;
}
// handleCCID


////////////////////////////////////////////////////////////////////////////////////////////////
void handleCsq(char* result) {
  int rssi;
  sendDataToBLE( result ) ;
  modemIsOk() ;
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
  modemIsOk() ;
}
// handleCellList


////////////////////////////////////////////////////////////////////////////////////////////////
void timeoutHandler() {
  oled.setCursor(0, 20);
  oled.print("GSM err 01");
  oled.display();

  sendDataToBLE("! GSM timeout") ;
  Serial.println(F("GSM not responding (timeout)"));
  GSMTimeout = true ;

  GSMErrorCount++ ;

  if( GSMErrorCount > MAX_GSM_ERROR_COUNT ) {
    sendDataToBLE("GSM: Restarting AT init procedure") ;
    Serial.println(F("Restarting AT init procedure"));
    initAT() ;
  }
}
// timeoutHandler



////////////////////////////////////////////////////////////////////////////////////////////////
void errorHandler() {
  oled.setCursor(0, 40);
  oled.print("GSM err 02");
  oled.display();
  GSMModemConnected = false ;

  sendDataToBLE("! GSM error") ;
  Serial.println(F("GSM Error"));
 
}
// errorHandler


void handleCmeError(char* result) {
  int errorCode;
  if (sscanf(result, "%d", &errorCode) == 1) {
    sendDataToBLE( "GSM: SME error" ) ;
    sendDataToBLE( result ) ;
    Serial.print(F("---- Specific Hardware/Network Error Code: "));
    Serial.println(errorCode);
    // Example: 10 = SIM not inserted, 11 = SIM PIN required, 30 = No network service
  }
}



// This catches specific SMS-related errors
void handleCmsError(char* result) {
  Serial.print(F("---- SMS Error details: "));
  Serial.println(result);
}


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

void modemIsOk() {
  GSMModemConnected = true ;
  GSMTimeout = false ;
  GSMErrorCount = 0 ;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Модем инициализирован и зарегистрирован в сети
bool isModemRegistered() {
  return initModeSate == MODEM_INIT_STEP_4 ;
}
// isModemRegistered

bool isGSMModemConnected() {
  return GSMModemConnected ;
}

bool isGSMTimeout() {
  return GSMTimeout ;
}


void requestSignalLevel() {
  gsmAsync.addCommand("AT+CSQ") ;
}
