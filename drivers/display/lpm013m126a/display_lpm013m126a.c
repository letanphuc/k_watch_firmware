/*
 * Copyright (c) 2024 JDI LPM013M126A Driver
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jdi_lpm013m126a

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_lpm013m126a, CONFIG_DISPLAY_LOG_LEVEL);

/* Commands */
#define LPM_CMD_UPDATE 0x90
#define LPM_CMD_ALL_CLEAR 0x20
#define LPM_CMD_NO_UPDATE 0x00
#define LPM_CMD_BLINKING_WHITE 0x18
#define LPM_CMD_BLINKING_BLACK 0x10
#define LPM_CMD_INVERSION 0x14

struct lpm013m126a_config {
  struct spi_dt_spec bus;
  struct gpio_dt_spec disp_en;
  struct gpio_dt_spec extcomin;
  struct pwm_dt_spec backlight;
  uint16_t width;
  uint16_t height;
};

struct lpm013m126a_data {
  uint8_t *buffer; /* Shadow buffer for 1-bit per pixel */
  bool polarity;
  struct k_work_delayable extcomin_work;
  const struct device *dev;
};

/* Buffer size: (width / 2) * height bytes for 4-bit mode (original driver logic)
   Wait, original driver used 4-bit per pixel logic but only 3 colors?
   Original: "Buffer: 176x176, 4bpp => 2 pixel/byte => 88 byte/line => ~15.5 KB"

   The display is 3-bit color (R, G, B).
   Original driver logic:
   idx = ((window_w / 2) * (y - window_y)) + ((x - window_x) / 2);
   if ((x & 1) == 0) { high nibble } else { low nibble }

   It seems the display expects 4 bits per pixel sent over SPI,
   regardless of how many are actually used.
   Data format: R G B x (x is dummy/unused?) or similar.
   Original defines:
   LCD_COLOR_BLACK (0x00)
   LCD_COLOR_BLUE (0x02)
   LCD_COLOR_GREEN (0x04)
   LCD_COLOR_CYAN (0x06)
   LCD_COLOR_RED (0x08)
   LCD_COLOR_MAGENTA (0x0a)
   LCD_COLOR_YELLOW (0x0c)
   LCD_COLOR_WHITE (0x0e)

   So it uses bits 1, 2, 3 for B, G, R. Bit 0 is 0.
*/

static void extcomin_toggle(struct k_work *work) {
  struct k_work_delayable *dwork = k_work_delayable_from_work(work);
  struct lpm013m126a_data *data = CONTAINER_OF(dwork, struct lpm013m126a_data, extcomin_work);
  const struct lpm013m126a_config *config = data->dev->config;

  if (device_is_ready(config->extcomin.port)) {
    gpio_pin_toggle_dt(&config->extcomin);
  }

  /* Toggle at least once per minute, doing it every second to be safe/simple */
  k_work_schedule(&data->extcomin_work, K_SECONDS(1));
}

static int lpm013m126a_blanking_on(const struct device *dev) {
  const struct lpm013m126a_config *config = dev->config;
  if (device_is_ready(config->disp_en.port)) {
    return gpio_pin_set_dt(&config->disp_en, 0);
  }
  return -ENOTSUP;
}

static int lpm013m126a_blanking_off(const struct device *dev) {
  const struct lpm013m126a_config *config = dev->config;
  if (device_is_ready(config->disp_en.port)) {
    return gpio_pin_set_dt(&config->disp_en, 1);
  }
  return -ENOTSUP;
}

static int lpm013m126a_set_brightness(const struct device *dev, const uint8_t brightness) {
  const struct lpm013m126a_config *config = dev->config;

  if (!device_is_ready(config->backlight.dev)) {
    return -ENOTSUP;
  }

  uint32_t period = config->backlight.period;
  uint32_t pulse = (period * brightness) / 255;  // API expects 0-255, we assume full range.

  /* Note: DTS might have PWM_POLARITY_INVERTED.
     If so, logic is handled by driver.
     Original driver did manual calculation based on user passed 0-100.
     Here we use brightness 0-255. */

  return pwm_set_dt(&config->backlight, period, pulse);
}

static int lpm013m126a_write(const struct device *dev, const uint16_t x, const uint16_t y,
                             const struct display_buffer_descriptor *desc, const void *buf) {
  const struct lpm013m126a_config *config = dev->config;
  struct lpm013m126a_data *data = dev->data;

  /* Check bounds logic skipped for brevity, assumed caller (LVGL) respects desc */

  /* Update shadow buffer
     We need to convert buffer (RGB565) to shadow buffer format.
     Since this is a memory LCD, we need to update the shadow buffer line by line
     or rect by rect, and then flush.
     However, the display command requires full line update.
     Best approach:
     1. Update the shadow buffer region corresponding to (x,y,w,h)
     2. Flush the dirty lines to the display.
  */

  uint16_t w = desc->width;
  uint16_t h = desc->height;

  /* 1. Update shadow buffer */
  /* Shadow buffer is full frame WxH/2 bytes */
  /* buf is W*H*2 bytes (RGB565) */

  const uint16_t *src = buf;

  for (int row = 0; row < h; row++) {
    uint16_t line_y = y + row;
    if (line_y >= config->height) break;

    for (int col = 0; col < w; col++) {
      uint16_t pixel_x = x + col;
      if (pixel_x >= config->width) break;

      uint16_t p = src[row * w + col];
      uint8_t r = (p & 0xF800) ? 1 : 0;
      uint8_t g = (p & 0x07E0) ? 1 : 0;
      uint8_t b = (p & 0x001F) ? 1 : 0;
      uint8_t val = (r << 3) | (g << 2) | (b << 1);

      /* Calculate index in shadow buffer */
      size_t idx = ((config->width / 2) * line_y) + (pixel_x / 2);

      if ((pixel_x & 1) == 0) {
        /* Even, high nibble */
        data->buffer[idx] &= 0x0F;
        data->buffer[idx] |= (val << 4);
      } else {
        /* Odd, low nibble */
        data->buffer[idx] &= 0xF0;
        data->buffer[idx] |= val;
      }
    }
    /* Log first 8 bytes of shadow buffer for this line */
    // if (row < 4) {
    //   char dbg[64];
    //   int dbg_len = 0;
    //   dbg_len += snprintk(dbg + dbg_len, sizeof(dbg) - dbg_len, "[L%u] BUF:", line_y);
    //   for (int i = 0; i < 8 && i < (int)line_data_len; ++i) {
    //     dbg_len += snprintk(dbg + dbg_len, sizeof(dbg) - dbg_len, " %02X", data->buffer[(config->width / 2) * line_y
    //     + i]);
    //   }
    //   LOG_DBG("%s", dbg);
    // }
  }

  /* 2. Flush dirty lines */
  /* Line format: CMD(1) + LINE(1) + DATA(88) + TRAILER(2) = 92 bytes?
     Original:
     CMD (1 byte)
     LINE (1 byte)
     DATA (88 bytes)
     TRAILER (2 bytes 0x00)

     But original uses `spi_packet_mixed` or manual control.
     Let's construct a buffer for one line transfer.
  */

  uint8_t line_buf[1 + 1 + (176 / 2) + 2]; /* Max possible size */
  uint32_t line_data_len = config->width / 2;

  struct spi_buf sbuf = {
      .buf = line_buf,
      .len = 2 + line_data_len + 2,
  };
  struct spi_buf_set tx = {
      .buffers = &sbuf,
      .count = 1,
  };

  for (int row = 0; row < h; row++) {
    uint16_t line_y = y + row;

    line_buf[0] = LPM_CMD_UPDATE | (data->polarity ? 0x40 : 0x00);
    line_buf[1] = line_y + 1; /* Line addresses are 1-based */

    memcpy(&line_buf[2], &data->buffer[(config->width / 2) * line_y], line_data_len);

    line_buf[2 + line_data_len] = 0x00;
    line_buf[2 + line_data_len + 1] = 0x00;

    /* Log first 8 bytes of line_buf for this line */
    if (row < 4) {
      char dbg[64];
      int dbg_len = 0;
      dbg_len += snprintk(dbg + dbg_len, sizeof(dbg) - dbg_len, "[L%u] TX:", line_y);
      for (int i = 0; i < 8 && i < (int)(2 + line_data_len + 2); ++i) {
        dbg_len += snprintk(dbg + dbg_len, sizeof(dbg) - dbg_len, " %02X", line_buf[i]);
      }
      LOG_DBG("%s", dbg);
    }

    int ret = spi_write_dt(&config->bus, &tx);
    if (ret < 0) {
      LOG_ERR("SPI write failed at line %u: %d", line_y, ret);
      return ret;
    }
  }

  return 0;
}

static int lpm013m126a_read(const struct device *dev, const uint16_t x, const uint16_t y,
                            const struct display_buffer_descriptor *desc, void *buf) {
  return -ENOTSUP;
}

static void *lpm013m126a_get_framebuffer(const struct device *dev) { return NULL; }

static int lpm013m126a_set_contrast(const struct device *dev, const uint8_t contrast) { return -ENOTSUP; }

static void lpm013m126a_get_capabilities(const struct device *dev, struct display_capabilities *caps) {
  const struct lpm013m126a_config *config = dev->config;

  caps->x_resolution = config->width;
  caps->y_resolution = config->height;
  caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565; /* We accept this and convert */
  caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
  caps->screen_info = SCREEN_INFO_MONO_VTILED | SCREEN_INFO_MONO_MSB_FIRST; /* Not really mono, but... */
                                                                            /* Set SCREEN_INFO_X_ALIGNMENT if needed */
}

static const struct display_driver_api lpm013m126a_api = {
    .blanking_on = lpm013m126a_blanking_on,
    .blanking_off = lpm013m126a_blanking_off,
    .write = lpm013m126a_write,
    .read = lpm013m126a_read,
    .get_framebuffer = lpm013m126a_get_framebuffer,
    .set_brightness = lpm013m126a_set_brightness,
    .set_contrast = lpm013m126a_set_contrast,
    .get_capabilities = lpm013m126a_get_capabilities,
};

static int lpm013m126a_init(const struct device *dev) {
  const struct lpm013m126a_config *config = dev->config;
  struct lpm013m126a_data *data = dev->data;

  data->dev = dev;

  if (!spi_is_ready_dt(&config->bus)) {
    LOG_ERR("SPI bus not ready");
    return -ENODEV;
  }

  if (device_is_ready(config->disp_en.port)) {
    gpio_pin_configure_dt(&config->disp_en, GPIO_OUTPUT_ACTIVE);
  }

  if (device_is_ready(config->extcomin.port)) {
    gpio_pin_configure_dt(&config->extcomin, GPIO_OUTPUT_INACTIVE);
    /* Start EXTCOMIN toggling */
    k_work_init_delayable(&data->extcomin_work, extcomin_toggle);
    k_work_schedule(&data->extcomin_work, K_SECONDS(1));
  }

  if (device_is_ready(config->backlight.dev)) {
    pwm_set_dt(&config->backlight, config->backlight.period, config->backlight.period); /* 100% on start */
  }

  /* Clear display */
  memset(data->buffer, 0xFF, (config->width / 2) * config->height);  // White

  uint8_t cmd_clear = LPM_CMD_ALL_CLEAR;
  uint8_t zero = 0x00;

  struct spi_buf buf[2] = {{.buf = &cmd_clear, .len = 1}, {.buf = &zero, .len = 1}};
  struct spi_buf_set tx = {.buffers = buf, .count = 2};

  spi_write_dt(&config->bus, &tx);

  return 0;
}

#define LPM_INIT(n)                                                                               \
  static uint8_t lpm_buf_##n[(DT_INST_PROP(n, width) / 2) * DT_INST_PROP(n, height)];             \
  static struct lpm013m126a_data lpm_data_##n = {                                                 \
      .buffer = lpm_buf_##n,                                                                      \
      .polarity = false,                                                                          \
  };                                                                                              \
  static const struct lpm013m126a_config lpm_config_##n = {                                       \
      .bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0), \
      .disp_en = GPIO_DT_SPEC_INST_GET_OR(n, disp_en_gpios, {0}),                                 \
      .extcomin = GPIO_DT_SPEC_INST_GET_OR(n, extcomin_gpios, {0}),                               \
      .backlight = PWM_DT_SPEC_INST_GET_OR(n, {0}),                                               \
      .width = DT_INST_PROP(n, width),                                                            \
      .height = DT_INST_PROP(n, height),                                                          \
  };                                                                                              \
  DEVICE_DT_INST_DEFINE(n, lpm013m126a_init, NULL, &lpm_data_##n, &lpm_config_##n, POST_KERNEL,   \
                        CONFIG_DISPLAY_INIT_PRIORITY, &lpm013m126a_api);

DT_INST_FOREACH_STATUS_OKAY(LPM_INIT)
