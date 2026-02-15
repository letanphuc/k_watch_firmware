#ifndef ANCS_CLIENT_H
#define ANCS_CLIENT_H

#include <bluetooth/services/ancs_client.h>
#include <zephyr/bluetooth/conn.h>

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

/* Struct for notification info */
typedef struct {
  char title[ATTR_DATA_SIZE];
  char message[ATTR_DATA_SIZE];
  char app[ATTR_DATA_SIZE];
} ancs_noti_info_t;

int ancs_client_init(void);

#endif  // ANCS_CLIENT_H
