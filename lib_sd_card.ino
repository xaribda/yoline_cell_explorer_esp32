// Пин на карт-ридере == плата 
// CLK (SCK)  GPIO 10 Тактовый сигнал
// MISO       GPIO 5 Вход данных
// MOSI       GPIO 6 Выход данных
// CS (SS)    GPIO 7 Выбор чипа
//
// Порядок пинов на модуле
// +3.3
// CS      -> #7
// MOSI    -> #6
// CLK     -> #10
// MISO    -> #5
// GRND


bool isSDCardRecording = false ;

SPIClass SDSPI(FSPI); 

// #define SD_PIN_SCK  10
// #define SD_PIN_MISO 5
// #define SD_PIN_MOSI 6
// #define SD_PIN_CS   7
#define SD_PIN_SCK  10
#define SD_PIN_MISO 5
#define SD_PIN_MOSI 6
#define SD_PIN_CS   7

fs::File root;

// После сканирования файлов вида data_nnnn.rec, maxFileNameID будет max(nnnn) 
int maxFileNameID = 0 ;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
bool SDRecordingMode = false ; // Записываем?
String SDFileName = "" ;
String ramSDBuffer = "" ;
unsigned long lastSaveTime = 0 ;
const size_t MAX_BUFFER_SIZE = 1024 ; // Запись при достижении 


/////////////////////////////////////////////////////////////////////////////
void initSD() {
  pinMode( SD_PIN_CS, OUTPUT ) ;
  digitalWrite( SD_PIN_CS, HIGH ) ;
  SDSPI.begin( SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS ) ;
  
  if( !SD.begin( SD_PIN_CS, SDSPI, 1000000 )) {
    Serial.println("! SD card problem, check wiring") ;
    return;
  } else {
    Serial.println("---------- FILES ON SD ---------------------------------------------");
    root = SD.open( "/" ) ;     
    printDirectory( root, 0 ) ;
    root.close() ; 
    Serial.println("---------- END FILES -----------------------------------------------");
    Serial.print( "Last file ID:" ) ; Serial.println( maxFileNameID ) ;
  }
}
// initSD


/////////////////////////////////////////////////////////////////////////////
// Как будет называться файл data_12345.rec
String generateNextFileName( int id ) {
  char buffer[6] ;
  sprintf( buffer, "%05d", id ) ;  
  return String("/data_") + buffer + String(".rec") ;
}
// generateNextFileName


/////////////////////////////////////////////////////////////////////////////
void loopSD() {
  if( once(5000)) {
    if (SD.cardSize() == 0) {
      Serial.println("! No SD card ") ;
    }
  }
}
// loopSD




/////////////////////////////////////////////////////////////////////////////
void printDirectory( fs::File dir, int numTabs) {
  maxFileNameID = 0 ;

  while (true) {
    File entry = dir.openNextFile();
    String fileName = entry.name() ;

    if (!entry) {
      // Файлов больше нет — выходим из цикла
      break;
    }
    
    // Делаем отступы (табуляцию) для красивого отображения структуры папок
    for (int i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    // Выводим имя файла или папки
    Serial.print( fileName );
    
    if (entry.isDirectory()) {
      Serial.println("/");

      // Если это папка, рекурсивно заходим в неё и увеличиваем отступ
      // printDirectory(entry, numTabs + 1);
    } else {
      // Если это файл, выводим его размер в байтах
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);

      int lastSlash = fileName.lastIndexOf('/') ; // Убрать '/'
      if( lastSlash != -1 ) {
        fileName = fileName.substring( lastSlash + 1 ) ;
      }

      // Проверяем, подходит ли файл 
      if( isDataFile( fileName )) {
          // Извлекаем строку с номером (между "data_" [5 символов] и ".rec")
          String numStr = fileName.substring( 5, fileName.length() - 4 ) ;
          int currentNumber = numStr.toInt() ;

          if( currentNumber > maxFileNameID ) {
              maxFileNameID = currentNumber ;
          }
      }
    }
    entry.close(); // Обязательно закрываем файл после чтения!
  } // while

}
// printDirectory


/////////////////////////////////////////////////////////////////////////////
// Проверяет, подходит ли имя файла под маску "data_XXXX.rec" 
bool isDataFile( String fileName ) {
  return ( fileName.startsWith("data_") && fileName.endsWith(".rec")) ; 
}
// isDataFile




/////////////////////////////////////////////////////////////////////////////
void toSDBuffer( String data ) {
  if( CDGPSDataString.length() == 0 ) return ;
  if( !isRecording() ) return ;

  ramSDBuffer+= data ;
  if( ramSDBuffer.length() >= MAX_BUFFER_SIZE ) {
    saveBufferToSD() ;
  }
}
// toSDBuffer


/////////////////////////////////////////////////////////////////////////////
void clearRecordingStatus() {
  oled.fillRect( 20, 0, 8, 8, BLACK ) ;
  oled.display() ;
}
// clearRecordingStatus

/////////////////////////////////////////////////////////////////////////////
void drawRecordingStatus() {
  drawWriteCDStatus( 20, 0 ) ;
  oled.display() ;
}
// clearRecordingStatus


/////////////////////////////////////////////////////////////////////////////
void saveBufferToSD() {
  if( ramSDBuffer.length() == 0 ) return ;
  drawRecordingStatus() ;
  oled.display() ;

  File file = SD.open( SDFileName, FILE_APPEND ) ;
  if( file ) {
    file.print( ramSDBuffer ) ;
    file.close() ;
    ramSDBuffer = "" ; 
    Serial.print("\\\\ Data saved to SD file: ") ;
    Serial.println( SDFileName ) ;
    lastSaveTime = millis() ;
  } else {
    Serial.println("\\\\ SD write error!") ;
  }

  drawRecordCDStatus( 20, 0 ) ;
  oled.display() ;
}
// saveBufferToSD


/////////////////////////////////////////////////////////////////////////////////
bool isRecording() {
  return isSDCardRecording ;
}
// isRecording


/////////////////////////////////////////////////////////////////////////////////
String getNewFileName() {
  maxFileNameID = 0 ;
  root = SD.open( "/" ) ;     

  while( true ) {
    File entry = root.openNextFile() ;
    String fileName = entry.name() ;
    if( !entry ) break ;
         
    if( !entry.isDirectory()) {
      int lastSlash = fileName.lastIndexOf('/') ; // Убрать '/'
      if( lastSlash != -1 ) fileName = fileName.substring( lastSlash + 1 ) ;

      // Проверяем, подходит ли файл 
      if( isDataFile( fileName )) {
          // Извлекаем строку с номером (между "data_" [5 символов] и ".rec")
          String numStr = fileName.substring( 5, fileName.length() - 4 ) ;
          int currentNumber = numStr.toInt() ;

          if( currentNumber > maxFileNameID ) maxFileNameID = currentNumber ;
      }
    }
    entry.close(); // Обязательно закрываем файл после чтения!
  } // while
  
  root.close() ; 
  maxFileNameID++ ;

  return generateNextFileName( maxFileNameID ) ;

}
// getNewFileName


/////////////////////////////////////////////////////////////////////////////////
// Начинаем запись на карту
String startRecord() {
  if( SD.cardSize() == 0 ) {
    return "No SD card" ;
  }

  if( getGNSSFixType() == 0 ) {
    return "No GNSS Fix" ;
  }

  if( !isGPS() ) {
    return "No GPS data (no connection)" ;
  }

  // Имя файла исходя из существующих data_12345.rec файлов +1
  SDFileName = getNewFileName() ;
  Serial.print( "Recording new file: ") ; Serial.print( SDFileName ) ;
  isSDCardRecording = true ;

  // Создаем файл и пишем заголовок
  File file = SD.open( SDFileName, FILE_APPEND ) ;

  String header = "<header>\n" ;
  header+= "device: " + getDeviceName() + "\n" ;
  header+= "version: " + getDeviceVersion() + "\n" ; 
  header+= "serial: " + getSerialFormatted() + "\n" ;
  header+= "start: " + GNSSDateTimeString() + "\n" ;
  header+= "</header>\n" ;
  
  file.print( header ) ;
  file.close() ;
  
  drawRecordingStatus() ;
  
  return "Recording is started, file: " + SDFileName ;
}
// startRecord


String stopRecord() {
  isSDCardRecording = false ;
  clearRecordingStatus() ; 
  return "Record stopped" ;
}

