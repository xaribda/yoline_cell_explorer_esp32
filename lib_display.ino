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
  int paramARCFNPosition = 0 ;
  int paramRXPosition = 1 ;
  int paramRXQPosition = 2 ; 

  if( dataString[0] == '0' && dataString[1] == ',' ) {
    paramCellIDPosition = 6 ;
  }   

  String cellid = parceParam( String( dataString ), paramCellIDPosition ) ; 
  String arcfn = parceParam( String( dataString ), paramARCFNPosition ) ; 
  String rxString = parceParam( String(dataString), paramRXPosition ); 
  String rxqString = parceParam( String(dataString), paramRXQPosition ); 
  int rx = rxString.toInt()  ;
  int rxq = rxqString.toInt() ;
    
  drawOneCell( id, cellid, arcfn, rx, rxq ) ;
  lastAchivedID = id ;

}
// displayOneCell


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawOneCell( int id, String cellid, String arcfn, int rx, int rxq ) {
  int y = DISPLAY_TOP_OFFSET + id * DISPLAY_CELL_INFO_HEIGHT ;
  oled.fillRect( 0, y, 128, DISPLAY_CELL_INFO_HEIGHT, BLACK ) ;

  oled.setCursor( 0, y ) ;
  if( cellid == "ffff" ) {
    oled.print( "----" ) ;
  } else {
    oled.print( cellid ) ;
  }

  oled.setCursor( 30, y ) ;
  oled.print( arcfn ) ;

  drawProgressBar( 0, y + 8, 25, 5, 63, rx ) ;
  drawProgressBar( 30, y + 8, 25, 5, 60, rxq ) ;
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