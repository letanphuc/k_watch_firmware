#ifndef BLE_HPP
#define BLE_HPP

#include <stdint.h>

class Ble {
 public:
  static Ble& instance() {
    static Ble inst;
    return inst;
  }

  /**
   * @brief Initialize the BLE subsystem and start advertising.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int init();

  void advertising_start();

 private:
  Ble() = default;
};

#endif /* BLE_HPP */
