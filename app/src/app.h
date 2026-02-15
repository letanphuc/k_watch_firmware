#ifndef APP_H
#define APP_H

#include "event.h"

int app_init(void);
uint32_t app_task_handler(void);
int app_event_post(app_event_t* event);

struct screen;
void app_switch_screen(struct screen* screen);

#endif /* APP_H */
