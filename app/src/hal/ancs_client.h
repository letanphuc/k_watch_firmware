#ifndef ANCS_CLIENT_H
#define ANCS_CLIENT_H

#include <bluetooth/services/ancs_client.h>
#include <zephyr/bluetooth/conn.h>

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

#define ATTR_TITLE_SIZE 64
#define ATTR_MESSAGE_SIZE 256
#define ATTR_APP_ID_SIZE 32
#define ATTR_COMMON_SIZE 32

/* Struct for notification info */
typedef struct {
  char* title;
  char* message;
  char* app;
} ancs_noti_info_t;

void ancs_noti_info_free(ancs_noti_info_t* noti);

int ancs_client_init(void);

#endif  // ANCS_CLIENT_H
