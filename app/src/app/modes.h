#ifndef MODES_H
#define MODES_H

#include <stdint.h>

typedef enum {
  APP_MODE_AMBIENT,
  APP_MODE_ACTIVE,
} app_mode_t;

void modes_init(void);

/**
 * @brief Notify the mode manager that activity (like a button press) has occurred.
 * This resets the 20-second timeout and ensures we are in ACTIVE mode.
 */
void modes_activity_detected(void);

/**
 * @brief Set the brightness level for ACTIVE mode.
 * @param brightness Percentage (0-100).
 */
void modes_set_active_brightness(uint8_t brightness);

/**
 * @brief Get current ACTIVE mode brightness.
 * @return Percentage (0-100).
 */
uint8_t modes_get_active_brightness(void);

#endif /* MODES_H */
