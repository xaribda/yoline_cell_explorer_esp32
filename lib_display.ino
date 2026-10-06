#define DISPLAY_TOP_OFFSET 16
#define DISPLAY_CELL_INFO_HEIGHT 16


int lastAchivedID = 0 ;


/////////////////////////////////////////////////////////////////////////////////////////////////
// 1,1 - игнорируем
// 0,"0975,27,99,250,02,52,ff3f,00,05,9d2c,255"
// 1,"0073,18,27,7f70,250,02,e7"
void displayOneCell(char* dataString) {
  if( dataString[1] == ',' && dataString[2] == '1' ) {
    //oled.clearDisplay() ;
    // Чистим оставшуюся часть экрана
    oled.fillRect( 0, DISPLAY_TOP_OFFSET + ( lastAchivedID + 1 ) * DISPLAY_CELL_INFO_HEIGHT, 128, 100, BLACK ) ;
    return ;
  }

  int id = dataString[0] - '0' ;
  int paramCellIDPosition = 3 ;
  int paramARFCNPosition = 0 ;
  int paramRXPosition = 1 ;
  int paramRXQPosition = 2 ; 

  if( dataString[0] == '0' && dataString[1] == ',' ) {
    paramCellIDPosition = 6 ;
  }   

  String cellid = parceParam( String( dataString ), paramCellIDPosition ) ; 
  String arfcn = parceParam( String( dataString ), paramARFCNPosition ) ; 
  String rxString = parceParam( String(dataString), paramRXPosition ); 
  String rxqString = parceParam( String(dataString), paramRXQPosition ); 
  int rx = rxString.toInt()  ;
  int rxq = rxqString.toInt() ;
    
  drawOneCell( id, cellid, arfcn, rx, rxq ) ;
  lastAchivedID = id ;

}
// displayOneCell


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawOneCell( int id, String cellid, String arfcn, int rx, int rxq ) {
  int y = DISPLAY_TOP_OFFSET + id * DISPLAY_CELL_INFO_HEIGHT ;
  oled.fillRect( 0, y, 128, DISPLAY_CELL_INFO_HEIGHT, BLACK ) ;

  oled.setCursor( 0, y ) ;
  if( cellid == "ffff" ) {
    oled.print( "----" ) ;
  } else {
    oled.print( cellid ) ;
  }

  oled.setCursor( 34, y ) ;
  oled.print( arfcn ) ;

  drawProgressBar( 0, y + 8, 30, 5, 63, rx ) ;
  drawProgressBar( 34, y + 8, 30, 5, 60, rxq ) ;
  oled.display() ;
}
// drawOneCell


/**
 * Извлекает n-й параметр из строки вида:
 * 0,"0975,27,99,250,02,52,ff3f,00,05,9d2c,255"
 *
 * @param input   исходная строка
 * @param index   номер параметра (0 = первый после кавычек)
 * @return        найденный параметр или пустая строка, если индекс неверный
 */
String parceParam(const String& input, int index) {
  // 1. Находим открывающую кавычку
  int startQuote = input.indexOf('"');
  if (startQuote < 0) return "";

  // 2. Находим закрывающую кавычку
  int endQuote = input.indexOf('"', startQuote + 1);
  if (endQuote < 0) return "";

  // 3. Вырезаем содержимое между кавычками
  String content = input.substring(startQuote + 1, endQuote);

  // 4. Разбиваем по запятым и берём нужный элемент
  int current = 0;
  int from = 0;

  while (true) {
    int comma = content.indexOf(',', from);

    if (comma < 0) {
      // последний параметр
      if (current == index) {
        return content.substring(from);
      }
      break;
    }

    if (current == index) {
      return content.substring(from, comma);
    }

    current++;
    from = comma + 1;
  }

  return ""; // индекс слишком большой
}
// parceParam


///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawProgressBar( int x, int y, int width, int height, int maxValue, int value ) {
  int percentage = int( value * 100 / maxValue ) ;

  // 1. Ensure percentage stays within 0 - 100 bounds
  percentage = constrain( percentage, 0, 100 ) ;
  int fillWidth = map( percentage, 0, 100, 0, width - 4) ;

  //oled.fillRect( x, y, width, height, BLACK ) ;
  oled.drawRect(x, y, width, height, WHITE ) ;
  if( fillWidth > 0 ) {
    oled.fillRect( x, y, fillWidth, height, WHITE ) ;
  }
}
// drawProgressBar



///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawBluethootStatus( int x, int y ) {
  const unsigned char bluetooth_icon[] PROGMEM = {
    0b00011000, 
    0b00100100, 
    0b01000010, 
    0b10000001, 
    0b10011001, 
    0b10011001, 
    0b11111111  
  } ;
  oled.drawBitmap(x, y, bluetooth_icon, 8, 7, SSD1306_WHITE) ;
}
// drawBluethootStatus


///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawWriteCDStatus( int x, int y ) {
  const unsigned char bluetooth_icon[] PROGMEM = {
    0b00011000, 
    0b01111110, 
    0b11111111, 
    0b11111111, 
    0b11111110, 
    0b01111110, 
    0b00011000  
  } ;
  oled.drawBitmap(x, y, bluetooth_icon, 8, 7, SSD1306_WHITE, SSD1306_BLACK ) ;
}
// drawWriteCDStatus


///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawRecordCDStatus( int x, int y ) {
  const unsigned char icon[] PROGMEM = {
    0b00011000, 
    0b01000010, 
    0b10000001, 
    0b10000001, 
    0b10000001, 
    0b01000010, 
    0b00011000  
  } ;
  oled.drawBitmap(x, y, icon, 8, 7, SSD1306_WHITE, SSD1306_BLACK ) ;
}
// drawRecordCDStatus



///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawSatteliteTypeStatus( int x, int y, int type ) {
  const unsigned char t0[] PROGMEM = {
    0b00000000, 
    0b00100000, 
    0b00000000, 
    0b00100000, 
    0b00000000, 
    0b00100000, 
    0b00000000  
  } ;

  const unsigned char t1[] PROGMEM = {
    0b01110000, 
    0b01110000, 
    0b01110000, 
    0b00000000, 
    0b00100000, 
    0b00000000, 
    0b00100000  
  } ;

  const unsigned char t2[] PROGMEM = {
    0b00100000, 
    0b00000000, 
    0b01110000, 
    0b01110000, 
    0b01110000, 
    0b00000000, 
    0b00100000  
  } ;

  const unsigned char t3[] PROGMEM = {
    0b00100000, 
    0b00000000, 
    0b00100000, 
    0b00000000, 
    0b01110000, 
    0b01110000, 
    0b01110000, 
  } ;

  if( type == 0 ) oled.drawBitmap(x, y, t0, 5, 7, SSD1306_WHITE ) ;
  if( type == 1 ) oled.drawBitmap(x, y, t1, 5, 7, SSD1306_WHITE ) ;
  if( type == 2 ) oled.drawBitmap(x, y, t2, 5, 7, SSD1306_WHITE ) ;
  if( type == 3 ) oled.drawBitmap(x, y, t3, 5, 7, SSD1306_WHITE ) ;
}
// drawSatteliteTypeStatus



///////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawDottedHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  for (int16_t i = 0; i < w; i += 2) { // Change 'i += 2' to 'i += 3' or more for larger spacing
    oled.drawPixel(x + i, y, color);
  }
}
// drawDottedHLine