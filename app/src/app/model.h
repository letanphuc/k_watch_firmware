#ifndef APP_MODEL_H
#define APP_MODEL_H

#include <stdint.h>

#include "../hal/ancs_client.h"

#define MAX_NOTIFICATIONS 10

void model_add_notification(const ancs_noti_info_t* noti);
uint8_t model_get_notification_count(void);
const ancs_noti_info_t* model_get_notification(uint8_t index);
void model_dump_notifications(void);
void modes_handle_timeout(void);

#endif  // APP_MODEL_H
