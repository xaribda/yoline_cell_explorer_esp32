bool isWebServerRunning() ;

// TODO сделать state uptime, state memory, state card (сколько места на карте)
// 
String executeCommand( String command ) {

  if( command.startsWith( "start web" )) {
    int channel = 1 ;
    String channelStr = command.substring(9) ;
    if( channelStr.length() > 0 ) {
      channel = channelStr.toInt() ;
    } else {
      channelStr = 1 ;
    }
    String password = startWebServer( channel ) ;
    return "WEB started, channel: " + channelStr + ", password: " + password ;
  }

  if( command == "stop web" ) {
    stopWebServer() ;
    return "WEB stopped" ;
  }

  if( command == "state web" ) {
    return isWebServerRunning() ? "WEB is running" : "WEB is stopped" ;
  }

  if( command == "get serial" ) {
    return getSerialFormatted() ;
  }

  if( command == "get name") {
    return getDeviceName() ;
  }

  if( command == "get version" ) {
    return getDeviceVersion() ;
  }

  if( command == "start record" ) {
    if( isRecording() ) return "Already recording" ;
    if( isWebServerRunning() ) stopWebServer() ;

    return startRecord() ;
  }

  if( command == "stop record" ) {
    return stopRecord() ;
  }

  if( command == "state record" ) {
    return isRecording() ? "Recording" : "Not recording" ;
  }


  if( command == "state sd" ) {
    return isRecording() ? "Recording" : "Not recording" ;
  }

  if( command == "get date" ) {
    return GNSSDateTimeString() ;
  }

  if( command == "get gnss" || command == F("get gps") ) {
    return getGNSSData() ;
  }

  if( command == "get battery" ) {
    return getBattery() ;
  }


  if( command == "uptime" ) {
    return "uptime seconds: " + String( (unsigned long) millis() / 1000 ) ;
  }

  if( command == "get memory" ) {
    return "free: " + String( ESP.getFreeHeap() ) ;
  }

  if( command == "get signal" ) {
    requestSignalLevel() ;
    return "sent" ;
  }

  if( command == "get profiles" ) {  
    if( isRecording() ) stopRecord() ;
    if( isWebServerRunning() ) stopWebServer() ;
    pauseRequestCellList() ;
    sendDataToBLE("Retrieving list...") ;
    String result = getSTKProfiles() ;
    startRequestCellList() ;
    return result ;
  }

  if( command.startsWith( "set profile" )) {
    if( isRecording() ) stopRecord() ;
    if( isWebServerRunning() ) stopWebServer() ;
    pauseRequestCellList() ;

    sendDataToBLE("Switching, please wait...") ;
    vTaskDelay(1) ;

    uint8_t profile = 0 ;
    String profilelStr = command.substring(12) ;
    if( profilelStr.length() > 0 && isDigit( profilelStr.charAt(0)) ) {
      profile = profilelStr.toInt() ;
    } else {
      profile = 1 ;
    }

    setSTKProfile( profile ) ;
    return "Profile [" + String( profile ) + "] activated" ;
  }


  if( command == "reboot" ) {
    if( isRecording() ) stopRecord() ;
    if( isWebServerRunning()) stopWebServer() ;
    sendDataToBLE("Rebooting now...") ;
    delay(200) ;
    ESP.restart() ;
    return "" ;
  }

  if( command == "reboot modem" ) {
    rebootModem() ;
    return F("Modem is rebooting") ;
  }

  if( command.startsWith( "call" )) {
    String phone = command.substring(5) ;
    if( phone.length() > 9 ) {
      callPhone( phone ) ;
      return "Calling: " + phone ;
    }

    return "Bad phone number" ;
  }


  return "command [" + command + "] not found" ;
}