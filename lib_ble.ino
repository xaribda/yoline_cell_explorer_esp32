#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


///////////////////////////////////////////////////////////////////////////////////////
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    oled.setCursor( 40, 0 ) ;
    oled.print( "BLE" ) ;
    Serial.println("--- connected --- ");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    oled.fillRect( 40, 0, 20, 10, BLACK ) ;
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
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Дескриптор для уведомлений
  pCharacteristic->addDescriptor( new BLE2902()) ;

  // Стартовое значение
  pCharacteristic->setValue("ESP32 is ready to communicate") ;

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
  //   pCharacteristic->setValue(data.c_str());
  //   pCharacteristic->notify();       

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
  if( !deviceConnected ) return ;

  Serial.print('BLE + ') ;

  //pCharacteristic->setValue( dataString ) ;
  pCharacteristic->setValue( (uint8_t*) dataString, strlen( dataString )) ;
  pCharacteristic->notify();       
}
// sendDataToBLE