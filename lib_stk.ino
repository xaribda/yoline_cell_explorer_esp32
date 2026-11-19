#include <eSIMBox_STK.h>
#include <esp_task_wdt.h>

EsimBoxSTK esim;


uint8_t STKLedColor = 10 ;
int STKDirection = 1 ;

/////////////////////////////////////////////////////////////////////////////////////////////////
void onEsimProgress() {
    esp_task_wdt_reset();

    STKLedColor += STKDirection ;
    if( STKLedColor > 128 || STKLedColor < 10 ) STKDirection = -STKDirection ;
    led( STKLedColor, 0, 0 ) ;

    // А то отвалится..
    loopBLE() ;
    vTaskDelay(1) ;
}
// onEsimProgress


/////////////////////////////////////////////////////////////////////////////////////////////////
void initSTK() {
  esim.begin( Serial0 ) ;
  esim.setProgressCallback( onEsimProgress ) ;
}
// initSTK


/////////////////////////////////////////////////////////////////////////////////////////////////
String getSTKProfiles() {
    String profileString = "List:\n" ;
    EsimBoxProfile profiles[8];
    size_t count = 0;

    if (esim.listProfiles(profiles, 8, count)) {
        Serial.println() ;
        
        for (int i = 0; i < count; i++) {
            profileString +=  "[" + profiles[i].id +  "] " + profiles[i].name + " " + ( profiles[i].enabled ? "[A]" : "" ) + "\n" ;
        }
    } else {
        profileString = esim.lastError() ;
    }

    return profileString ;
}
// getSTKProfiles


/////////////////////////////////////////////////////////////////////////////////////////////////
void setSTKProfile( uint8_t profile ) {
    esp_task_wdt_init( 10, true ) ; 

    Serial.println("setSTKProfile") ;
    delay(100) ;
    if( !esim.enableProfile( profile )) {
        Serial.print( "Error: ") ;
        Serial.println( esim.lastError() ) ;
    }

    Serial.println("startInitModemSequence") ;
    delay(100) ;
    startInitModemSequence() ;

  esp_task_wdt_init( WDT_TIMEOUT, true ) ; 
}
// setSTKProfile