#include "esp_wifi.h"
#include "esp_bt.h"
#include <DNSServer.h>

AsyncWebServer server(80) ;
DNSServer dnsServer ;
const byte DNS_PORT = 53 ;

String webPassword ;
bool webServerRunning = false ;
bool softAPRunning = false;

const char* WEB_USERNAME = "admin" ;

// Объявляем предварительно
String generatePassword( uint8_t ) ;
String fileYMDHM( File ) ;
bool isWebServerRunning() ;
String webPageHTMLStart() ;
String webPageHTMLEnd() ;
String formatBytes(size_t) ;

///////////////////////////////////////////////////////////////////////////////////////
void initWEB() {
  //esp_bt_controller_disable(); 
  //esp_wifi_set_max_tx_power( WIFI_POWER_8_5dBm ) ;
  //delay(100) ;
  // Выключаем, экономим энергию
  //WiFi.mode( WIFI_OFF ) ; 
  //WiFi.mode( WIFI_STA ) ;

}
// initWEB  


void loopWEB() {
  if (softAPRunning) {
    dnsServer.processNextRequest();   // ← очень важно!
  }  
}
// loopWEB


String startWebServer( int channel ) {
  if (webServerRunning) {
    return webPassword;
  }

  webPassword = generatePassword(4);

  // --- SoftAP + Captive Portal ---
  WiFi.disconnect(true, true); 
  delay( 200 ) ; 
  WiFi.mode( WIFI_STA ) ;
  delay( 200 ) ; 
  WiFi.mode( WIFI_AP ) ;
  delay( 200 ) ; 
  //WiFi.setTxPower( WIFI_POWER_19_5dBm ) ; // Максимум!!!
  WiFi.setTxPower( WIFI_POWER_8_5dBm ) ; 
  delay( 200 ) ; 

  
  // Для Android Captive Portal
  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig( apIP, gateway, subnet);

  bool ok = WiFi.softAP( getDeviceName().c_str(), NULL, channel ) ;


  if (ok) {
    softAPRunning = true;
    
    // DNS: все домены → IP ESP32 (это и есть основа captive portal)
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    
    Serial.println("SoftAP + Captive Portal started") ;
    Serial.printf("  SSID:     %s\n", getDeviceName().c_str() ) ;
    Serial.printf("  Password: %s\n", webPassword.c_str()) ;
    Serial.print  ("  IP:       ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start SoftAP");
  }

  // --- Основные страницы ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!checkAuth(request)) return;
    request->send(200, "text/html", webPageMain() );
  });

  server.on("/bypass", HTTP_GET, [](AsyncWebServerRequest *request) {
    dnsServer.stop() ;
    Serial.println("----- handleBypass" ) ;

    // 2. Отправляем заголовок редиректа на нужную страницу
    request->redirect("http://192.168.4.1/files");
  }) ;

  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", webPageFiles() );
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!checkAuth(request)) return;

    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filepath = request->getParam("file")->value();
    
    if (filepath.indexOf("..") >= 0) {
      request->send(403, "text/plain", "Forbidden");
      return;
    }

    if (!SD.exists(filepath)) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    request->send(SD, filepath, "application/octet-stream", true);
  });

  // --- Captive Portal: перенаправляем всё на главную ---
  // Android
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    // НЕ возвращаем 204! Иначе Android решит, что интернет есть
    request->redirect("http://192.168.4.1/");
  });

  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });
  
  // Apple
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });
  
  // Windows
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });
  
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  // Всё остальное тоже на главную
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.begin();
  webServerRunning = true;

  Serial.printf("Web server started. Password: %s\n", webPassword.c_str());
  return webPassword;
}


////////////////////////////////////////////////////////////////////////////////////////////////
void stopSPI() {
  SPI.end(); // Полностью останавливаем аппаратный SPI

  // Переводим пины картридера в режим INPUT, чтобы они не фонили
  pinMode(SD_PIN_SCK, INPUT);
  pinMode(SD_PIN_MISO, INPUT);
  pinMode(SD_PIN_MOSI, INPUT);
  pinMode(SD_PIN_CS, INPUT);  
}
// stopSPI()


// ====================== Остановка веб-сервера ======================
void stopWebServer() {
  if( !webServerRunning ) return ;
  server.end() ;
  
  if( softAPRunning ){
    WiFi.softAPdisconnect( true ) ;
    Serial.println("SoftAP stopped") ;
  }
  
  // Выключаем, экономим энергию
  WiFi.disconnect( true, true ) ; 
  WiFi.mode( WIFI_OFF ) ; 
  Serial.println("WiFi stopped") ;

  webPassword = "" ;
  webServerRunning = false;
  softAPRunning = false ;

  Serial.println("Web server stopped") ;
}
// stopWebServer


String webPageMain() {
  Serial.println("----- webPageMain" ) ;
  return webPageHTMLStart() + webPageHTMLFileList( true ) + webPageHTMLEnd() ; 
}
// webPageMain


String webPageFiles() {
  Serial.println("----- webPageFiles" ) ;

  return webPageHTMLStart() + webPageHTMLFileList( false ) + webPageHTMLEnd() ; 
}
// webPageMain


///////////////////////////////////////////////////////////////////////////////////////
bool checkAuth( AsyncWebServerRequest *request ) {
  if (!request->authenticate( WEB_USERNAME, webPassword.c_str())) {
    request->requestAuthentication( "YOLINE FR Fly Scanner", false ) ; 
    return false;
  }
  return true;
}
// checkAuth


///////////////////////////////////////////////////////////////////////////////////////
String webPageHTMLFileList( bool isCaptive ) {
  String html = R"rawliteral(
    <table>
      <thead>
        <tr>
          <th>File</th><th>Size</th><th>Date</th>
        </tr>
      </thead>
      <tbody>
  )rawliteral" ; 
  
  File root = SD.open("/") ;
  if( !root || !root.isDirectory() ) {
    html += "<tr><td colspan='3' class='empty'>SD card is not ready</td></tr>" ;
  } else {
    bool hasFiles = false ;
    File file = root.openNextFile() ;
    
    while (file) {
      if (!file.isDirectory() ) {
        hasFiles = true;
        String name = String(file.name());
        if( name[0] == '.' ) { 
          file.close() ;
          file = root.openNextFile() ;
          continue ;
        }

        // Убираем ведущий слэш, если есть
        if (name.startsWith("/")) name = name.substring(1);
        
        html += "<tr>";
        if( isCaptive ) {
          html += "<td>" + name + "</td>";
        } else {
          html += "<td><a href=\"/download?file=/" + name + "\" download=\"" + name + "\">" + name + "</a></td>";
        }
        html += "<td class='size'>" + formatBytes(file.size()) + "</td>";
        html += "<td>" + fileYMDHM( file ) + "</td>" ;
        html += "</tr>";
      }
      file.close() ;
      file = root.openNextFile() ;
    } // while

    root.close() ;
    
    if (!hasFiles) {
      html += "<tr><td colspan='3' class='empty'>No files on SD card</td></tr>" ;
    }
  }

  html += "</tbody></table>" ;
  if( isCaptive ) {
    html += "<div style=\"text-align:center;padding:20px;\"><a href='http://192.168.4.1/files'>Enter the system</a></div>" ;
  }
  return html ;
}
// webPageHTMLFileList


///////////////////////////////////////////////////////////////////////////////////////
String webPageHTMLStart() {
  return R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <title>Yoline RF Fly Scanner</title>
            <style>
              body { font-family: system-ui, -apple-system, sans-serif; max-width: 800px; margin: 20px auto; padding: 0 15px; background: #f5f5f5; }
              h1 { color: #333; }
              .card { background: white; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); overflow: hidden; }
              table { width: 100%; border-collapse: collapse; }
              table th { background: #1a73e8; color: white; padding: 12px 15px; text-align: left; font-size:12px; }
              table td { padding: 10px 15px; border-bottom: 1px solid #eee; font-size:12px; }
              tr:hover { background: #f8f9fa; }
              a { color: #1a73e8; text-decoration: none; font-weight: 500; }
              a:hover { text-decoration: underline; }
              .size { color: #666; font-size: 0.9em; }
              .empty { padding: 40px; text-align: center; color: #999; }
            </style>
          </head>
          <body>
            <h3>)rawliteral" + getDeviceName() + R"rawliteral(</h3>
            <div class="card">
          )rawliteral" ;  
}
// webPageHTMLStart


///////////////////////////////////////////////////////////////////////////////////////
String webPageHTMLEnd() {
  return "</div></body></html>" ;
}
// webPageHTMLEnd


// ====================== Генерация пароля ======================
String generatePassword(uint8_t length = 4) {
  const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  String pass;
  pass.reserve(length);
  
  for (uint8_t i = 0; i < length; i++) {
    pass += charset[esp_random() % (sizeof(charset) - 1)];
  }
  return pass;
}

///////////////////////////////////////////////////////////////////////////////////////
String formatBytes(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
  else return String(bytes / 1024.0 / 1024.0, 2) + " MB";
}
// formatBytes

bool isWebServerRunning() {
  return webServerRunning ;
}

///////////////////////////////////////////////////////////////////////////////////////
String fileYMDHM( File file ) {
  time_t lastWriteTime = file.getLastWrite() ;
  struct tm *timeinfo = localtime( &lastWriteTime ) ;
  char formattedDateTime[20];
  strftime( formattedDateTime, sizeof(formattedDateTime), "%d.%m.%Y %H:%M", timeinfo ) ;
 

  return String( formattedDateTime ) ;
}
// fileYMDHM