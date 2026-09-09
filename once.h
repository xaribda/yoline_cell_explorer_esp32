#ifndef ONCE_TIMER_H
#define ONCE_TIMER_H

#include <Arduino.h>

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

#endif // ONCE_TIMER_H