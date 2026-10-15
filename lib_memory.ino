

uint32_t memoryBefore ;

void m1() {
  memoryBefore = ESP.getFreeHeap() ;
}


void m2() {
  if( memoryBefore > ESP.getFreeHeap() ) {
    Serial.print(F("------------- memory leak detected: ")) ;
    Serial.print( memoryBefore - ESP.getFreeHeap() ) ;
  }
}