#ifndef APP_SCREEN_H
#define APP_SCREEN_H

#include <stdint.h>

#include "../event.h"

typedef struct screen {
  void (*init)(void);
  void (*handle_event)(app_event_t* event);
  void (*load)(void);
} screen_t;

#endif  // APP_SCREEN_H
