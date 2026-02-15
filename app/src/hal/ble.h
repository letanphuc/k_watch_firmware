#ifndef BLE_H
#define BLE_H

/**
 * @brief Initialize the BLE subsystem and start advertising.
 *
 * This function initializes the Bluetooth stack, registers authentication callbacks,
 * initializes CTS and ANCS clients, loads settings, and starts advertising.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ble_init(void);

#endif /* BLE_H */
