#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <zephyr/kernel.h>

int app_init(void);
uint32_t app_task_handler(void);

extern struct k_sem lvgl_refresh_sem;

#endif /* APP_H */
