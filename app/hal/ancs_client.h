#ifndef ANCS_CLIENT_H
#define ANCS_CLIENT_H

#include <stdint.h>

#define ATTR_DATA_SIZE 32

typedef struct {
    char title[ATTR_DATA_SIZE];
    char message[ATTR_DATA_SIZE];
    char app[ATTR_DATA_SIZE];
} ancs_noti_info_t;

#endif // ANCS_CLIENT_H
