// Макрос __LINE__ автоматически создает уникальную статическую переменную 
// для каждой строчки, где вызвана функция в коде
#define once(ms) \
  ([](unsigned long interval) -> bool { \
    static unsigned long lastMillis = 0; \
    unsigned long currentMillis = millis(); \
    if (currentMillis - lastMillis >= interval) { \
      lastMillis = currentMillis; \
      return true; \
    } \
    return false; \
  })(ms)