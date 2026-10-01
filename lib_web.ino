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
const char* SOFTAP_SSID  = "YOLINE";

String generatePassword( uint8_t ) ;

///////////////////////////////////////////////////////////////////////////////////////
void initWEB() {
  //esp_bt_controller_disable(); 
  //esp_wifi_set_max_tx_power( WIFI_POWER_8_5dBm ) ;
  //delay(100) ;
  WiFi.mode( WIFI_AP ) ;
  delay(100) ;
}
// initWEB  


void loopWEB() {
  if (softAPRunning) {
    dnsServer.processNextRequest();   // ← очень важно!
  }  
}
// loopWEB


String startWebServer() {
  if (webServerRunning) {
    return webPassword;
  }

  webPassword = generatePassword(4);

  // --- SoftAP + Captive Portal ---
    WiFi.mode( WIFI_STA ) ;
    delay( 500 ) ; 
    WiFi.mode( WIFI_AP ) ;
    
    //bool ok = WiFi.softAP(SOFTAP_SSID, webPassword.c_str());
    bool ok = WiFi.softAP(SOFTAP_SSID, NULL, 11 );
    
    if (ok) {
      softAPRunning = true;
      
      // DNS: все домены → IP ESP32 (это и есть основа captive portal)
      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      
      Serial.println("SoftAP + Captive Portal started");
      Serial.printf("  SSID:     %s\n", SOFTAP_SSID);
      Serial.printf("  Password: %s\n", webPassword.c_str());
      Serial.print  ("  IP:       ");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("Failed to start SoftAP");
    }

  // --- Основные страницы ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!checkAuth(request)) return;
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
    request->redirect("/");
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

///////////////////////////////////////////////////////////////////////////////////////
// Запуск веб-сервера
String startWebServer2() {
  if (webServerRunning) {
    return webPassword;   // уже запущен — возвращаем текущий пароль
  }
  webPassword = generatePassword(8);

  //Serial.print("WiFi status: ") ; Serial.println( WiFi.status() ) ;
  // Прогрев!!
  WiFi.mode( WIFI_STA ) ;
  delay( 500 ) ; 
  WiFi.mode( WIFI_AP ) ;
  
  // Пароль SoftAP = тот же 4-символьный пароль
  //bool ok = WiFi.softAP(SOFTAP_SSID, webPassword.c_str(), 11 ) ;  
  bool ok = WiFi.softAP(SOFTAP_SSID, NULL, 11 ) ;  

  if( ok ){
    softAPRunning = true;
    Serial.println("SoftAP started");
    Serial.printf("  SSID: %s\n", SOFTAP_SSID);
    Serial.printf("  Password: %s\n", webPassword.c_str());
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start SoftAP");
  }

  // Главная страница — список файлов
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    request->send(200, "text/html", webPageFiles());
  });

  // Скачивание файла
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filepath = request->getParam("file")->value();
    
    // Защита от path traversal
    if (filepath.indexOf("..") >= 0) {
      request->send(403, "text/plain", "Forbidden");
      return;
    }

    if (!SD.exists(filepath)) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    // Отдаём файл с правильным именем
    request->send(SD, filepath, "application/octet-stream", true);
  });

  // 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404. Page not found");
  });

  server.begin();
  webServerRunning = true;

  Serial.printf("Web server started. Password: %s\n", webPassword.c_str());
  return webPassword;
}
// startWebServer


// ====================== Остановка веб-сервера ======================
void stopWebServer() {
  if (!webServerRunning) return;

  server.end();
  webServerRunning = false;
  
  if( softAPRunning ){
    WiFi.softAPdisconnect( true ) ;
    softAPRunning = false ;
    Serial.println("SoftAP stopped") ;
  }

  webPassword = "" ;
  
  Serial.println("Web server stopped") ;
}
// stopWebServer


String webPageFiles() {
  return webPageHTMLStart() + webPageHTMLFileList() + webPageHTMLEnd() ; 
}
// webPageFiles


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
String webPageHTMLFileList() {
  String html = "" ;
  
  File root = SD.open("/") ;
  if( !root || !root.isDirectory() ) {
    html += "<tr><td colspan='3' class='empty'>SD card is not ready</td></tr>" ;
  } else {
    bool hasFiles = false ;
    File file = root.openNextFile() ;
    
    while (file) {
      if (!file.isDirectory()) {
        hasFiles = true;
        String name = String(file.name());
        // Убираем ведущий слэш, если есть
        if (name.startsWith("/")) name = name.substring(1);
        
        html += "<tr>";
        html += "<td>" + name + "</td>";
        html += "<td class='size'>" + formatBytes(file.size()) + "</td>";
        html += "<td><a href=\"/download?file=/" + name + "\">download</a></td>";
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
              th { background: #1a73e8; color: white; padding: 12px 15px; text-align: left; }
              td { padding: 10px 15px; border-bottom: 1px solid #eee; }
              tr:hover { background: #f8f9fa; }
              a { color: #1a73e8; text-decoration: none; font-weight: 500; }
              a:hover { text-decoration: underline; }
              .size { color: #666; font-size: 0.9em; }
              .empty { padding: 40px; text-align: center; color: #999; }
            </style>
          </head>
          <body>
            <h1>Please select file to upload</h1>
            <div class="card">
              <table>
                <thead>
                  <tr>
                    <th>File</th>
                    <th>Size</th>
                    <th></th>
                  </tr>
                </thead>
                <tbody>
          )rawliteral" ;  
}
// webPageHTMLStart


///////////////////////////////////////////////////////////////////////////////////////
String webPageHTMLEnd() {
  return R"rawliteral(
      </tbody>
    </table>
  </div>
</body>
</html>
)rawliteral"; ;
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