// Пин на карт-ридереВаша плата 
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


SPIClass SDSPI(FSPI); 

#define SD_PIN_SCK  10
#define SD_PIN_MISO 5
#define SD_PIN_MOSI 6
#define SD_PIN_CS   7

fs::File root;

void initSD() {
  SDSPI.begin( SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS ) ;

  if( !SD.begin( SD_PIN_CS, SDSPI )) {
    Serial.println("! SD card problem, check wiring") ;
    return;
  } else {
    Serial.println("---------- FILES ON SD ----------------------");
    root = SD.open( "/" ) ;     
    printDirectory( root, 0 ) ;
    root.close() ; 
    Serial.println("---------------------------------------------");
  }
}


void loopSD() {
  if( once(5000)) {
    if (SD.cardSize() == 0) {
      Serial.println("! No SD card ") ;
    }
  }
}


void printDirectory( fs::File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // Файлов больше нет — выходим из цикла
      break;
    }
    
    // Делаем отступы (табуляцию) для красивого отображения структуры папок
    for (int i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    // Выводим имя файла или папки
    Serial.print(entry.name());
    
    if (entry.isDirectory()) {
      Serial.println("/");
      // Если это папка, рекурсивно заходим в неё и увеличиваем отступ
      printDirectory(entry, numTabs + 1);
    } else {
      // Если это файл, выводим его размер в байтах
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close(); // Обязательно закрываем файл после чтения!
  }
}