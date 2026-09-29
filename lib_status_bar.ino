/////////////////////////////////////////////////////////////////////////////////////
void loopStatusBar() {
  if( once( 2000 )) {
    batteryStatus() ;
    gnssStatus() ;
  }

  if( once( 10000 )) {
    drawStatusPlacement() ;
  }

}
// loopStatusBar


/////////////////////////////////////////////////////////////////////////////////////
void drawStatusPlacement() {
    drawDottedHLine( 0, 10, 64, WHITE ) ;
}
// drawStatusPlacement


/////////////////////////////////////////////////////////////////////////////////////
void gnssStatus() {
  oled.fillRect( 0, 0, 30, 10, BLACK ) ;
  oled.setCursor( 0, 0 ) ;

  oled.setCursor( 0, 0 ) ;
  if( GPSSatteliteCount < 10 ) oled.print( "0" ) ;
  oled.print( GPSSatteliteCount ) ; 

  drawSatteliteTypeStatus( 13, 0, GPSFixType ) ;

  //oled.print( ":" ); oled.print( GPSFixType ) ; 
}
// gnssStatus 


/////////////////////////////////////////////////////////////////////////////////////
void batteryStatus() {
  if( chgStatus > 0 && batteryPercent > 0 ) return batteryStatusCharging() ;

  oled.fillRect( 45, 0, 20, 10, BLACK ) ;
  oled.setCursor( 45, 0 ) ;

  if( batteryPercent > 0 ) {
    if( batteryPercent < 100 ) {
      oled.print( batteryPercent ) ;
      oled.print( "%" ) ;
    } else {
      oled.print( "100" ) ;
    }
  } else {
    oled.print("???") ;
  }

}
// batteryStatus


/////////////////////////////////////////////////////////////////////////////////////
void batteryStatusCharging() {
  oled.setTextColor( BLACK, WHITE ); 

  oled.fillRect( 45, 0, 20, 10, WHITE ) ;
  oled.setCursor( 45, 0 ) ;

  if( batteryPercent < 100 ) {
    oled.print( batteryPercent, BLACK ) ;
    oled.print( "%" ) ;
  } else {
    oled.print( "100" ) ;
  }
  oled.setTextColor( WHITE, BLACK ) ; 
}