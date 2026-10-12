#include <sys/time.h>
#include <time.h>

// Arduino IDE автоматически создаёт прототипы функций для файлов .ino перед
// компиляцией. Это явное объявление не даёт IDE создать прототип
// setGNSSData() раньше, чем будет объявлена структура UBX_NAV_PVT_Payload.
struct UBX_NAV_PVT_Payload ;
void setGNSSData(UBX_NAV_PVT_Payload* pvt) ;

// ============================================================================
// Публичные данные GNSS
// ============================================================================

double GPSLatitude = 0.0 ;
double GPSLongitude = 0.0 ;
int GPSAccuracy = 0 ;                 // мм, по данным GNSS
int GPSFixType = 0 ;
int GPSSatteliteCount = 0 ;
bool weGotDataFromGPS = false ;
String CDGPSDataString = "" ;

int32_t GNSSHeight = 0 ;
uint16_t GNSSYear = 0 ;
uint8_t  GNSSMonth = 0 ;
uint8_t  GNSSDay = 0 ;
uint8_t  GNSSHour = 0 ;
uint8_t  GNSSSMin = 0 ;
uint8_t  GNSSSSec = 0 ;

// ============================================================================
// Настройки фильтра
// ============================================================================

// GNSS-приёмник может работать на частоте 10 Гц, но приложение получает стабильную позицию
// только раз в 1 секунду. Установить 2000 для выдачи раз в 2 секунды.
const uint32_t GNSS_OUTPUT_INTERVAL_MS = 1000 ;

// Не принимаем решение с очень низкой заявленной горизонтальной точностью.
const uint32_t GNSS_MAX_HACC_MM = 25000 ;       // 25 м

// Отбрасываем одиночную точку, если она настолько далеко от отфильтрованной позиции,
// что такое перемещение физически маловероятно.
const double GNSS_MAX_JUMP_METERS = 50.0 ;

// Дополнительный запас для одной эпохи навигации при 10 Гц.
const double GNSS_JUMP_MARGIN_METERS = 5.0 ;

// Количество хороших измерений для устойчивого сглаживания.
const uint8_t GNSS_FILTER_SAMPLES = 7 ;

// Минимальное количество измерений, после которого позиция считается достоверной.
const uint8_t GNSS_MIN_FILTER_SAMPLES = 3 ;

// Позиция выдаётся только с этим интервалом; внутренний фильтр при этом работает
// на каждом корректном пакете NAV-PVT.
const uint32_t GNSS_STALE_TIMEOUT_MS = 3000 ;

// ============================================================================
// Внутреннее состояние фильтра
// ============================================================================

double filteredLat = 0.0 ;
double filteredLon = 0.0 ;
bool filteredPositionValid = false ;

double sampleLat[GNSS_FILTER_SAMPLES] ;
double sampleLon[GNSS_FILTER_SAMPLES] ;
uint8_t sampleCount = 0 ;
uint8_t sampleIndex = 0 ;

uint32_t lastOutputMs = 0 ;
uint32_t lastGoodFixMs = 0 ;
uint32_t lastPvtMs = 0 ;

int32_t latestVelN = 0 ;       // мм/с
int32_t latestVelE = 0 ;       // мм/с
int32_t latestGSpeed = 0 ;     // мм/с
uint32_t latestSAcc = 0 ;      // мм/с
uint16_t latestPDOP = 0 ;      // сотые доли


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Настройка UBX
// Пакет UBX-CFG-VALSET: включение UBX на UART1 (оставлен для справки)
const uint8_t enableNmea[] = {
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x20, 0x40, 0xF5, 0x93
} ;

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Пакет UBX-CFG-VALSET: отключение UBX на UART1
const uint8_t disableUbx[] = {
  0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x40, 0xF6, 0x9C
} ;

//////////////////////////////////////////////////////////////////////////////////////////////////////
String getGNSSDataAndDateTimeString() {
  return getGNSSData() + " " + GNSSDateTimeFormattedString() ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
int getGNSSFixType() {
  return GPSFixType ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void initGPS() {
  gpsHardwareSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN) ;
  delay(200) ;
  configureGPS_UBX() ;

  // Даём приёмнику немного времени на начало выдачи NAV-PVT.
  delay(100) ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void loopGPS() {
  // Считываем все доступные данные из GNSS UART как можно быстрее.
  loopGPS1() ;

  if(once(10000)) {
    loopGPS2() ;
  }

  // Выдаём отфильтрованную позицию раз в 1 секунду.
  if(once(1000)) {
    loopGPS3() ;
  }

  if(once(5000)) {
    syncTime() ;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Считываем все доступные байты GNSS.
void loopGPS1() {
  while(gpsHardwareSerial.available() > 0) {
    weGotDataFromGPS = true ;

    uint8_t incomingByte = gpsHardwareSerial.read() ;
    processUBXByte(incomingByte) ;
  }

  yield() ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Мониторинг
void loopGPS2() {
  if(!weGotDataFromGPS) {
    sendDataToBLE("! GPS not found (not connected)") ;
    Serial.println("! GPS not found (not connected)") ;
  }

  if(!filteredPositionValid) {
    sendDataToBLE("! No stable GPS position") ;
    Serial.println("! No stable GPS position") ;
  } else if(lastPvtMs != 0 && (uint32_t)(millis() - lastPvtMs) > GNSS_STALE_TIMEOUT_MS) {
    sendDataToBLE("! GPS data stale") ;
    Serial.println("! GPS data stale") ;
  }

  weGotDataFromGPS = false ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Отправка отфильтрованной позиции в BLE
void loopGPS3() {
  if(filteredPositionValid && GPSFixType > 0) {
    CDGPSDataString = getGNSSData() ;
    Serial.println(CDGPSDataString) ;
    sendGNSSToBLE(CDGPSDataString.c_str()) ;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
String getGNSSData() {
  char buffer[180] ;

  snprintf( buffer, sizeof(buffer), "$fix [%.7f,%.7f] Sat: [%d] fixType: [%d] acc: [%d] speed: [%ld] PDOP: [%u] height: [%d] valid: [%d]", 
  GPSLatitude, GPSLongitude, GPSSatteliteCount, GPSFixType, GPSAccuracy, (long)latestGSpeed, latestPDOP, GNSSHeight, filteredPositionValid ) ;

  return String(buffer) ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
String GNSSDateTimeFormattedString() {
  if(GNSSYear == 0)
    return "date: [xx.xx.xxxx] time:[xx:xx:xx]" ;

  char buffer[100] ;

  snprintf(buffer, sizeof(buffer), "date: [%02d.%02d.%04d] time: [%02d:%02d:%02d]", GNSSDay, GNSSMonth, GNSSYear, GNSSHour, GNSSSMin, GNSSSSec) ;

  return String(buffer) ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
String GNSSDateTimeString() {
  if(GNSSYear == 0)
    return "xx.xx.xxxx xx:xx:xx" ;

  char buffer[20] ;

  snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d %02d:%02d:%02d", GNSSDay, GNSSMonth, GNSSYear, GNSSHour, GNSSSMin, GNSSSSec) ;

  return String(buffer) ;
}

// ============================================================================
// UBX NAV-PVT
// ============================================================================

struct __attribute__((packed)) UBX_NAV_PVT_Payload {
  uint32_t iTOW ;
  uint16_t year ;
  uint8_t  month ;
  uint8_t  day ;
  uint8_t  hour ;
  uint8_t  min ;
  uint8_t  sec ;
  uint8_t  valid ;
  uint32_t tAcc ;
  int32_t  nano ;
  uint8_t  fixType ;
  uint8_t  flags ;
  uint8_t  flags2 ;
  uint8_t  numSV ;
  int32_t  lon ;
  int32_t  lat ;
  int32_t  height ;
  int32_t  hMSL ;
  uint32_t hAcc ;
  uint32_t vAcc ;
  int32_t  velN ;
  int32_t  velE ;
  int32_t  velD ;
  int32_t  gSpeed ;
  int32_t  headMot ;
  uint32_t sAcc ;
  uint32_t headAcc ;
  uint16_t pDOP ;
  uint8_t  flags3 ;
  uint8_t  reserved1[5] ;
  int32_t  headVeh ;
  int16_t  magDec ;
  uint16_t magAcc ;
} ;

static_assert(
  sizeof(UBX_NAV_PVT_Payload) == 92,
  "UBX_NAV_PVT_Payload must be 92 bytes"
) ;

// ============================================================================
// Парсер UBX
// ============================================================================

enum UBX_STATE {
  SYNC1,
  SYNC2,
  CLASS,
  ID,
  LENGTH_L,
  LENGTH_H,
  PAYLOAD,
  CHK_A,
  CHK_B
} ;

UBX_STATE ubxState = SYNC1 ;

uint8_t msgClass = 0 ;
uint8_t msgId = 0 ;
uint16_t payloadLength = 0 ;
uint16_t payloadCounter = 0 ;
uint8_t calcCK_A = 0 ;
uint8_t calcCK_B = 0 ;

uint8_t payloadBuffer[100] ;

//////////////////////////////////////////////////////////////////////////////////////////////////////
void processUBXByte(uint8_t b) {
  switch(ubxState) {

    case SYNC1:
      if(b == 0xB5)
        ubxState = SYNC2 ;
      break ;

    case SYNC2:
      if(b == 0x62) {
        ubxState = CLASS ;
        calcCK_A = 0 ;
        calcCK_B = 0 ;
        payloadLength = 0 ;
      } else {
        ubxState = (b == 0xB5) ? SYNC2 : SYNC1 ;
      }
      break ;

    case CLASS:
      msgClass = b ;
      calcCK_A += b ;
      calcCK_B += calcCK_A ;
      ubxState = ID ;
      break ;

    case ID:
      msgId = b ;
      calcCK_A += b ;
      calcCK_B += calcCK_A ;
      ubxState = LENGTH_L ;
      break ;

    case LENGTH_L:
      payloadLength = b ;
      calcCK_A += b ;
      calcCK_B += calcCK_A ;
      ubxState = LENGTH_H ;
      break ;

    case LENGTH_H:
      payloadLength |= ((uint16_t)b << 8) ;
      calcCK_A += b ;
      calcCK_B += calcCK_A ;
      payloadCounter = 0 ;

      if(payloadLength > sizeof(payloadBuffer)) {
        ubxState = SYNC1 ;
      } else {
        ubxState = (payloadLength == 0) ? CHK_A : PAYLOAD ;
      }
      break ;

    case PAYLOAD:
      payloadBuffer[payloadCounter++] = b ;
      calcCK_A += b ;
      calcCK_B += calcCK_A ;

      if(payloadCounter >= payloadLength)
        ubxState = CHK_A ;
      break ;

    case CHK_A:
      if(b == calcCK_A)
        ubxState = CHK_B ;
      else
        ubxState = SYNC1 ;
      break ;

    case CHK_B:
      if(b == calcCK_B) {
        if(msgClass == 0x01 && msgId == 0x07) {
          UBX_NAV_PVT_Payload* pvt = (UBX_NAV_PVT_Payload*)payloadBuffer ;
          setGNSSData(pvt) ;
        }
      }

      ubxState = SYNC1 ;
      break ;
  }
}

// ============================================================================
// Вспомогательные функции фильтра позиции
// ============================================================================

//////////////////////////////////////////////////////////////////////////////////////////////////////
static double metersPerDegreeLat() {
  return 111320.0 ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
static double metersPerDegreeLon(double latitudeDeg) {
  double latRad = latitudeDeg * DEG_TO_RAD ;
  double value = 111320.0 * cos(latRad) ;

  // Избегаем численных проблем возле полюсов.
  if(value < 1.0)
    value = 1.0 ;

  return value ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
static double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  double dLat = (lat2 - lat1) * metersPerDegreeLat() ;
  double dLon = (lon2 - lon1) * metersPerDegreeLon((lat1 + lat2) * 0.5) ;

  return sqrt(dLat * dLat + dLon * dLon) ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Медиана сохранённых измерений широты/долготы.
//
// Медиана используется намеренно вместо обычного среднего: одно ошибочное GNSS
// измерение не сможет сильно сместить результат от основной группы точек.
//////////////////////////////////////////////////////////////////////////////////////////////////////
static double medianOf(double* values, uint8_t count) {
  double temp[GNSS_FILTER_SAMPLES] ;

  for( uint8_t i = 0 ; i < count ; i++ )
    temp[i] = values[i] ;

  for( uint8_t i = 0 ; i < count ; i++ ) {
    for( uint8_t j = i + 1 ; j < count ; j++ ) {
      if(temp[j] < temp[i]) {
        double t = temp[i] ;
        temp[i] = temp[j] ;
        temp[j] = t ;
      }
    }
  }

  if(count == 0)
    return 0.0 ;

  if(count & 1)
    return temp[count / 2] ;

  return (temp[count / 2 - 1] + temp[count / 2]) * 0.5 ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
static void resetPositionFilter( double lat, double lon) {
  sampleCount = 0 ;
  sampleIndex = 0 ;

  for( uint8_t i = 0 ; i < GNSS_FILTER_SAMPLES ; i++ ) {
    sampleLat[i] = 0.0 ;
    sampleLon[i] = 0.0 ;
  }

  filteredLat = lat ;
  filteredLon = lon ;
  filteredPositionValid = false ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
static bool acceptGNSSSample( double lat, double lon, uint32_t hAcc, uint8_t fixType, uint8_t flags) {
  // u-blox рекомендует проверять флаг gnssFixOK.
  if((flags & 0x01) == 0)
    return false ;

  // Для горизонтальных координат достаточно 2D-фикса.
  if(fixType < 2)
    return false ;

  // Отбрасываем явно плохие решения.
  if(hAcc == 0 || hAcc > GNSS_MAX_HACC_MM)
    return false ;

  if(!filteredPositionValid)
    return true ;

  const uint32_t now = millis() ;
  double dt = (lastGoodFixMs == 0) ? 0.1 :
              (double)(now - lastGoodFixMs) / 1000.0 ;

  // Защищаемся от зависания основного цикла или большой паузы от приёмника.
  if(dt < 0.05) dt = 0.05 ;
  if(dt > 2.0) dt = 2.0 ;

  double distance = distanceMeters(filteredLat, filteredLon, lat, lon) ;
  double speedMps = latestGSpeed / 1000.0 ;

  // Ожидаемое перемещение + запас с учётом точности. Это гораздо менее
  // произвольно, чем фиксированный порог скачка 30 м: движущийся приёмник может
  // реально переместиться дальше неподвижного.
  double allowedMovement = speedMps * dt + GNSS_JUMP_MARGIN_METERS ;
  double accuracyAllowance = max(2.0, 3.0 * (double)hAcc / 1000.0) ;
  double maxAllowed = max(GNSS_MAX_JUMP_METERS,
                          allowedMovement + accuracyAllowance) ;

  return distance <= maxAllowed ;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Добавляет одно хорошее GNSS-измерение в устойчивый фильтр.
//////////////////////////////////////////////////////////////////////////////////////////////////////
static void addGNSSSample( double lat, double lon ) {
  if(sampleCount < GNSS_FILTER_SAMPLES) {
    sampleLat[sampleCount] = lat ;
    sampleLon[sampleCount] = lon ;
    sampleCount++ ;
  } else {
    sampleLat[sampleIndex] = lat ;
    sampleLon[sampleIndex] = lon ;

    sampleIndex++ ;
    if(sampleIndex >= GNSS_FILTER_SAMPLES)
      sampleIndex = 0 ;
  }

  double medianLat = medianOf(sampleLat, sampleCount) ;
  double medianLon = medianOf(sampleLon, sampleCount) ;

  // При запуске ждём несколько независимых хороших измерений.
  if( sampleCount < GNSS_MIN_FILTER_SAMPLES) {
    filteredLat = medianLat ;
    filteredLon = medianLon ;
    return ;
  }

  // Адаптивное сглаживание:
  //
  // Медленное или плохо известное движение -> более сильное сглаживание.
  // Более быстрое движение -> меньше сглаживания для уменьшения задержки.
  double speedMps = latestGSpeed / 1000.0 ;
  double alpha ;

  if( speedMps < 0.5)
    alpha = 0.20 ;
  else if( speedMps < 2.0)
    alpha = 0.35 ;
  else if( speedMps < 5.0)
    alpha = 0.55 ;
  else
    alpha = 0.75 ;

  if( !filteredPositionValid ){
    filteredLat = medianLat ;
    filteredLon = medianLon ;
    filteredPositionValid = true ;
  } else {
    filteredLat += (medianLat - filteredLat) * alpha ;
    filteredLon += (medianLon - filteredLon) * alpha ;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Обрабатывает один корректный пакет NAV-PVT.
//
// Важно: приложение по-прежнему получает GNSS с исходной частотой приёмника
// (обычно 10 Гц). Фильтрация и выдача позиции не зависят от частоты приёмника.
//////////////////////////////////////////////////////////////////////////////////////////////////////
void setGNSSData(UBX_NAV_PVT_Payload* pvt) {
  double rawLat = pvt->lat / 10000000.0 ;
  double rawLon = pvt->lon / 10000000.0 ;

  GPSFixType = pvt->fixType ;
  GPSSatteliteCount = pvt->numSV ;
  GPSAccuracy = pvt->hAcc ;

  latestVelN = pvt->velN ;
  latestVelE = pvt->velE ;
  latestGSpeed = pvt->gSpeed ;
  latestSAcc = pvt->sAcc ;
  latestPDOP = pvt->pDOP ;

  GNSSHeight = pvt->hMSL ;

  GNSSYear = pvt->year ;
  GNSSMonth = pvt->month ;
  GNSSDay = pvt->day ;
  GNSSHour = pvt->hour ;
  GNSSSMin = pvt->min ;
  GNSSSSec = pvt->sec ;

  // Сохраняем доступную временную метку даже если позиция была отклонена.
  weGotDataFromGPS = true ;

  if(!acceptGNSSSample(rawLat, rawLon, pvt->hAcc, pvt->fixType, pvt->flags)) {
    return ;
  }

  lastGoodFixMs = millis() ;

  addGNSSSample( rawLat, rawLon ) ;

  // Храним отфильтрованное состояние внутри фильтра. Публичные GPSLatitude/GPSLongitude
  // обновляются только с заданным интервалом выдачи.
  uint32_t now = millis() ;
  lastPvtMs = now ;

  if( filteredPositionValid &&
      (lastOutputMs == 0 ||
       (uint32_t)(now - lastOutputMs) >= GNSS_OUTPUT_INTERVAL_MS)) {
    GPSLatitude = filteredLat ;
    GPSLongitude = filteredLon ;
    lastOutputMs = now ;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Настройка приёмника
// Сообщаем GNSS, что нужен UBX-NAV-PVT на UART1.
//
// Приёмник продолжает работать с настроенной частотой измерений/выдачи.
// ESP32 фильтрует позицию независимо, поэтому нет необходимости
// снижать частоту обновления GNSS только из-за того, что приложению нужно 1-2 Гц.
//////////////////////////////////////////////////////////////////////////////////////////////////////
void configureGPS_UBX() {
  uint8_t msg[] = {
    0xB5, 0x62,
    0x06, 0x01,
    0x08, 0x00,

    0x01,             // класс сообщения = NAV
    0x07,             // идентификатор сообщения = PVT
    0x00,             // частота I2C
    0x01,             // частота UART1 = 1
    0x00,             // частота UART2
    0x00,             // частота USB
    0x00,             // частота SPI
    0x00              // резерв
  } ;

  uint8_t ckA = 0 ;
  uint8_t ckB = 0 ;

  for( int i = 2 ; i < (int)sizeof(msg) ; i++ ) {
    ckA += msg[i] ;
    ckB += ckA ;
  }

  gpsHardwareSerial.write(msg, sizeof(msg)) ;
  gpsHardwareSerial.write(ckA) ;
  gpsHardwareSerial.write(ckB) ;

  gpsHardwareSerial.flush() ;

  Serial.printf("GPS: UBX-NAV-PVT enabled, CK=%02X %02X\n", ckA, ckB) ;
}

// ============================================================================
// Системное время
// ============================================================================

//////////////////////////////////////////////////////////////////////////////////////////////////////
void syncTime() {
  if(GNSSYear > 0) {
    syncSystemTimeWithGPS(GNSSYear, GNSSMonth, GNSSDay, GNSSHour, GNSSSMin, GNSSSSec) ;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void syncSystemTimeWithGPS(int year, int month, int day, int hour, int minute, int second) {
  struct tm t = {} ;

  t.tm_year = year - 1900 ;
  t.tm_mon = month - 1 ;
  t.tm_mday = day ;
  t.tm_hour = hour ;
  t.tm_min = minute ;
  t.tm_sec = second ;
  t.tm_isdst = 0 ;

  char *old_tz = getenv("TZ") ;

  setenv("TZ", "UTC0", 1) ;
  tzset() ;

  time_t epochTime = mktime(&t) ;

  if( old_tz ) {
    setenv("TZ", old_tz, 1) ;
  } else {
    unsetenv("TZ") ;
  }

  tzset() ;

  struct timeval tv ;
  tv.tv_sec = epochTime ;
  tv.tv_usec = 0 ;

  if( settimeofday(&tv, NULL) != 0 ) {
    Serial.println("Ошибка установки времени!") ;
  }
}
