#ifndef LED_HPP
#define LED_HPP

#include <stdint.h>

class Led {
 public:
  static Led& instance() {
    static Led inst;
    return inst;
  }

  int init();
  int set_color(uint8_t r, uint8_t g, uint8_t b);

 private:
  Led() = default;
};

#endif /* LED_HPP */
