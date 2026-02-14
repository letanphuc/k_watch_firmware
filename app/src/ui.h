#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <zephyr/kernel.h>

int ui_init(void);
uint32_t ui_task_handler(void);
void ui_watchface_update(void);

extern struct k_sem lvgl_refresh_sem;

#endif /* UI_H */
