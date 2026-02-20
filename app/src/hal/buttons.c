#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "event.h"

LOG_MODULE_REGISTER(buttons);

#define BUTTON_NODE(i) DT_ALIAS(sw##i)

#define BUTTON_SPEC(i) GPIO_DT_SPEC_GET(BUTTON_NODE(i), gpios)
#define BUTTON_LABEL(i) DT_PROP(BUTTON_NODE(i), label)

#define NUM_BUTTONS 4

static const struct gpio_dt_spec buttons[NUM_BUTTONS] = {
    BUTTON_SPEC(0),
    BUTTON_SPEC(1),
    BUTTON_SPEC(2),
    BUTTON_SPEC(3),
};

static struct gpio_callback button_cb_data[NUM_BUTTONS];

static const char* button_labels[NUM_BUTTONS] = {
    BUTTON_LABEL(0),
    BUTTON_LABEL(1),
    BUTTON_LABEL(2),
    BUTTON_LABEL(3),
};

static void button_pressed(const struct device* dev, struct gpio_callback* cb, uint32_t pins) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (cb == &button_cb_data[i]) {
      app_event_t event = {
          .type = APP_EVENT_BUTTON,
          .value = i,
          .len = 0,
      };
      LOG_INF("%s / %d pressed", button_labels[i], i);
      event_post(&event);
    }
  }
}

static int init_button(int idx) {
  int ret;
  const struct gpio_dt_spec* btn = &buttons[idx];

  if (!gpio_is_ready_dt(btn)) {
    LOG_ERR("%s device not ready", button_labels[idx]);
    return -ENODEV;
  }

  ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
  if (ret < 0) {
    LOG_ERR("Failed to configure %s: %d", button_labels[idx], ret);
    return ret;
  }

  ret = gpio_pin_interrupt_configure_dt(btn, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Failed to configure %s interrupt: %d", button_labels[idx], ret);
    return ret;
  }

  gpio_init_callback(&button_cb_data[idx], button_pressed, BIT(btn->pin));
  gpio_add_callback(btn->port, &button_cb_data[idx]);

  LOG_INF("%s initialized", button_labels[idx]);
  return 0;
}

int buttons_init(void) {
  int ret;

  for (int i = 0; i < NUM_BUTTONS; i++) {
    ret = init_button(i);
    if (ret < 0) {
      return ret;
    }
  }

  return 0;
}
