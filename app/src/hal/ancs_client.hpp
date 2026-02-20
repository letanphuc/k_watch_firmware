#ifndef ANCS_CLIENT_HPP
#define ANCS_CLIENT_HPP

#include <stdint.h>

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

class AncsClient {
 public:
  static AncsClient& instance() {
    static AncsClient inst;
    return inst;
  }

  int init();

 private:
  AncsClient() = default;
};

#endif /* ANCS_CLIENT_HPP */
