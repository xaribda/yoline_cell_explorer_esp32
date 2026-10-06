bool isWebServerRunning() ;

// TODO сделать state uptime, state memory, 
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

  if( command == F("stop web") ) {
    stopWebServer() ;
    return "WEB stopped" ;
  }

  if( command == F("state web") ) {
    return isWebServerRunning() ? "WEB is running" : "WEB is stopped" ;
  }

  if( command == F("get serial") ) {
    return getSerialFormatted() ;
  }

  if( command == F("get name")) {
    return getDeviceName() ;
  }

  if( command == F("get version") ) {
    return getDeviceVersion() ;
  }

  if( command == F("start record") ) {
    if( isRecording() ) return "Already recording" ;
    if( isWebServerRunning() ) stopWebServer() ;

    return startRecord() ;
  }

  if( command == F("stop record") ) {
    return stopRecord() ;
  }

  if( command == F("state record") ) {
    return isRecording() ? "Recording" : "Not recording" ;
  }

  if( command == F("get date") ) {
    return GNSSDateTimeString() ;
  }

  if( command == F("get gnss") || command == F("get gps") ) {
    return getGNSSData() ;
  }

  if( command == F("get battery") ) {
    return getBattery() ;
  }


  if( command == F("reboot") ) {
    if( isRecording() ) stopRecord() ;
    if( isWebServerRunning()) stopWebServer() ;
    sendDataToBLE("Rebooting now...") ;
    delay(200) ;
    ESP.restart() ;
    return "" ;
  }


  return "command [" + command + "] not found" ;
}