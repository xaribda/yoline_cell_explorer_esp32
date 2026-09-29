// ESP32C3 dev module
// CPU 160Mhz
#define SERVICE_UUID               "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_COMMON_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a1"
#define CHARACTERISTIC_GNSS_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a2"

// BLE каналы и т.п.
BLEServer* pServer = nullptr ;
BLECharacteristic* pCommonCharacteristic = nullptr ;
BLECharacteristic* pGNSSCharacteristic = nullptr ;
bool deviceConnected = false ;
bool oldDeviceConnected = false ;


///////////////////////////////////////////////////////////////////////////////////////
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    // oled.setCursor( 40, 0 ) ;
    // oled.print( "BLE" ) ;
    drawBluethootStatus( 32,0 ) ;
    Serial.println("--- connected --- ");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    oled.fillRect( 32, 0, 8, 8, BLACK ) ;
    Serial.println("--- disconnected --- ");
  }
} ;
// MyServerCallbacks


///////////////////////////////////////////////////////////////////////////////////////
void initBLE() {
  BLEDevice::init("YOLINE Cell Explorer") ;
  BLEDevice::setMTU(512) ;

  // Создание сервера
  pServer = BLEDevice::createServer() ;
  pServer->setCallbacks(new MyServerCallbacks()) ;

  // Создание сервиса
  BLEService *pService = pServer->createService( SERVICE_UUID ) ;

  // Создание характеристики (чтение + уведомления)
  pCommonCharacteristic = pService->createCharacteristic( CHARACTERISTIC_COMMON_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY ) ;
  pGNSSCharacteristic = pService->createCharacteristic( CHARACTERISTIC_GNSS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY ) ;

  // Дескриптор для уведомлений
  pCommonCharacteristic->addDescriptor( new BLE2902()) ;
  pGNSSCharacteristic->addDescriptor( new BLE2902()) ;

  // Стартовое значение
  pCommonCharacteristic->setValue("YOLINE Cell Explorer is ready to communicate") ;

  // Запуск сервиса
  pService->start();

  // Настройка представления (advertising)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising() ;
  pAdvertising->addServiceUUID(SERVICE_UUID) ;
  pAdvertising->setScanResponse(true) ;
  pAdvertising->setMinPreferred(0x06) ;  // помогает на iPhone
  pAdvertising->setMinPreferred(0x12) ;
  BLEDevice::startAdvertising() ;  
}
// initBLE


///////////////////////////////////////////////////////////////////////////////////////
void loopBLE() {
  // if( once(2000) && deviceConnected) {
  //   // Пример: отправляем счётчик
  //   String data = "Value: hi hi" ;
  //   pCommonCharacteristic->setValue(data.c_str());
  //   pCommonCharacteristic->notify();       

  //   Serial.println("Отправлено: " + data);
  // }  

  // Если отключились — снова начинаем рекламу
  if( !deviceConnected && oldDeviceConnected ) {
    delay(500); // даём стеку BLE время
    pServer->startAdvertising();
    Serial.println("Снова рекламируем...");
    oldDeviceConnected = deviceConnected;
  }

  // Если только что подключились
  if( deviceConnected && !oldDeviceConnected ) {
    oldDeviceConnected = deviceConnected;
  }
}
// loopBLE


///////////////////////////////////////////////////////////////////////////////////////
void sendDataToBLE( char* dataString ) {
  if( !deviceConnected ) { 
    Serial.println( "BLE: no connected devices") ;
    return ;
  }

  // Serial.println() ;
  // Serial.print('BLE + ') ;
  // Serial.println( dataString ) ;

  //pCommonCharacteristic->setValue( dataString ) ;
  pCommonCharacteristic->setValue( (uint8_t*) dataString, strlen( dataString )) ;
  pCommonCharacteristic->notify();       
}
// sendDataToBLE


///////////////////////////////////////////////////////////////////////////////////////
void sendGNSSToBLE( char* dataString ) {
  if( !deviceConnected ) return ;

  pGNSSCharacteristic->setValue( (uint8_t*) dataString, strlen( dataString )) ;
  pGNSSCharacteristic->notify();       
}
// sendDataToBLE