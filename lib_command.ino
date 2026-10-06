bool isWebServerRunning() ;


String executeCommand( String command ) {

  if( command.startsWith("start web" )) {
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

  if( command == "get name" ) {
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

  if( command == "stop record" || command == "sr" ) {
    return stopRecord() ;
  }

  if( command == "state record" ) {
    return isRecording() ? "Recording" : "Not recording" ;
  }

  if( command == "get date" ) {
    return GNSSDateTimeString() ;
  }

  if( command == "get gnss" || command == "get gps" ) {
    return getGNSSData() ;
  }

  if( command == "get battery" ) {
    return getBattery() ;
  }


  if( command == "reboot" ) {
    if( isRecording() ) stopRecord() ;
    if( isWebServerRunning()) stopWebServer() ;
    ESP.restart() ;
    return "rebooting" ;
  }


  return "command [" + command + "] not found" ;
}