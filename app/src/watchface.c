#include <zephyr/logging/log.h>

#include "misc/lv_area.h"
LOG_MODULE_REGISTER(watchface);
#include <lvgl.h>

#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "misc/lv_color.h"
#include "rtc.h"
#include "watchface.h"

static lv_obj_t *label_hour;
static lv_obj_t *label_colon;
static lv_obj_t *label_minute;
static lv_obj_t *label_date;
static lv_obj_t *icon_ampm;
static lv_obj_t *date_labels[7];
static lv_obj_t *cont;
static lv_obj_t *row_time;
static lv_obj_t *row_date;
static lv_obj_t *row_matrix;
static lv_obj_t *label_battery_percent;
static lv_obj_t *battery_icon;
LV_FONT_DECLARE(seven_segments_64);
LV_FONT_DECLARE(font_vi_20);

const lv_color_t color_blue = LV_COLOR_MAKE(0, 0, 255);
const lv_color_t color_red = LV_COLOR_MAKE(255, 0, 0);
const lv_color_t color_green = LV_COLOR_MAKE(0, 255, 0);
const lv_color_t color_cyan = LV_COLOR_MAKE(0, 255, 255);
const lv_color_t color_magenta = LV_COLOR_MAKE(255, 0, 255);
const lv_color_t color_yellow = LV_COLOR_MAKE(255, 255, 0);
const lv_color_t color_white = LV_COLOR_MAKE(255, 255, 255);
const lv_color_t color_black = LV_COLOR_MAKE(0, 0, 0);

extern const lv_image_dsc_t battery_status_0;
extern const lv_image_dsc_t battery_status_1;
extern const lv_image_dsc_t battery_status_2;
extern const lv_image_dsc_t battery_status_3;
extern const lv_image_dsc_t battery_status_4;
extern const lv_image_dsc_t battery_status_5;
extern const lv_image_dsc_t sun;
extern const lv_image_dsc_t moon;

const lv_image_dsc_t *battery_icons[] = {
    &battery_status_0,  // empty
    &battery_status_1,  // 20%
    &battery_status_2,  // 40%
    &battery_status_3,  // 60%
    &battery_status_4,  // 80%
    &battery_status_5,  // full
};

void watchface_init(void) {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, color_white, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // Main flex container
  cont = lv_obj_create(scr);
  lv_obj_set_size(cont, 176, 176);
  lv_obj_center(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_top(cont, 8, 0);
  lv_obj_set_style_pad_bottom(cont, 8, 0);

  // Row 0: Battery status icon (right aligned, with percentage label)
  lv_obj_t *row_battery = lv_obj_create(cont);
  lv_obj_set_size(row_battery, 176, 56);
  lv_obj_set_style_bg_opa(row_battery, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(row_battery, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_battery, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  battery_icon = lv_img_create(row_battery);
  lv_img_set_src(battery_icon, battery_icons[0]);
  lv_obj_align(battery_icon, LV_ALIGN_CENTER, 0, 0);

  label_battery_percent = lv_label_create(row_battery);
  lv_label_set_text(label_battery_percent, "100%");
  lv_obj_set_style_text_color(label_battery_percent, color_black, 0);
  lv_obj_set_style_text_font(label_battery_percent, &lv_font_montserrat_14, 0);
  lv_obj_set_style_pad_left(label_battery_percent, 2, 0);
  lv_obj_set_style_text_align(label_battery_percent, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_align(label_battery_percent, LV_ALIGN_CENTER, 0, 0);

  // Row 1: Time
  row_time = lv_obj_create(cont);
  lv_obj_set_size(row_time, 176, 60);
  lv_obj_set_style_bg_opa(row_time, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row_time, 0, 0);
  lv_obj_set_flex_flow(row_time, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_time, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Hour label in red round rect, white text
  lv_obj_t *hour_container = lv_obj_create(row_time);
  lv_obj_set_size(hour_container, 72, 60);
  lv_obj_set_style_bg_color(hour_container, color_red, 0);
  lv_obj_set_style_radius(hour_container, 8, 0);
  lv_obj_set_style_bg_opa(hour_container, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hour_container, 0, 0);
  label_hour = lv_label_create(hour_container);
  lv_label_set_text(label_hour, "00");
  lv_obj_set_style_text_color(label_hour, color_white, 0);
  lv_obj_set_style_text_font(label_hour, &seven_segments_64, 0);
  lv_obj_set_style_pad_all(label_hour, 4, 0);
  lv_obj_align(label_hour, LV_ALIGN_CENTER, 0, 0);

  // Colon label, red text (same style as minute)
  label_colon = lv_label_create(row_time);
  lv_label_set_text(label_colon, ":");
  lv_obj_set_style_text_color(label_colon, color_red, 0);
  lv_obj_set_style_text_font(label_colon, &seven_segments_64, 0);
  lv_obj_align(label_colon, LV_ALIGN_CENTER, 0, 0);

  // Minute label, red text
  label_minute = lv_label_create(row_time);
  lv_label_set_text(label_minute, "00");
  lv_obj_set_style_text_color(label_minute, color_red, 0);
  lv_obj_set_style_text_font(label_minute, &seven_segments_64, 0);
  lv_obj_align(label_minute, LV_ALIGN_CENTER, 0, 0);

  // Row 2: Date with AM/PM icon
  row_date = lv_obj_create(cont);
  lv_obj_set_size(row_date, 176, 24);
  lv_obj_set_style_bg_opa(row_date, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row_date, 0, 0);
  lv_obj_set_flex_flow(row_date, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_date, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  icon_ampm = lv_img_create(row_date);
  lv_img_set_src(icon_ampm, &sun);  // Default to sun
  lv_obj_set_size(icon_ampm, 24, 24);
  lv_obj_align(icon_ampm, LV_ALIGN_CENTER, 0, 0);

  label_date = lv_label_create(row_date);
  lv_label_set_text(label_date, "Jan 1, 2026");
  lv_obj_set_style_text_font(label_date, &font_vi_20, 0);
  lv_obj_set_style_text_color(label_date, color_blue, 0);
  lv_obj_set_style_text_align(label_date, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(label_date, LV_ALIGN_RIGHT_MID, 0, 0);

  // Row 3: Matrix button (dow + date)
  row_matrix = lv_obj_create(cont);
  lv_obj_set_size(row_matrix, 176, 32);
  lv_obj_set_style_bg_opa(row_matrix, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row_matrix, 0, 0);
  lv_obj_set_flex_flow(row_matrix, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(row_matrix, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Date row only
  lv_obj_t *date_row = lv_obj_create(row_matrix);
  lv_obj_set_size(date_row, 176, 32);
  lv_obj_set_style_bg_opa(date_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(date_row, 0, 0);
  lv_obj_set_flex_flow(date_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(date_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  for (int i = 0; i < 7; ++i) {
    date_labels[i] = lv_label_create(date_row);
    lv_label_set_text(date_labels[i], "00");
    lv_obj_set_style_text_color(date_labels[i], color_black, 0);
    lv_obj_set_style_text_font(date_labels[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(date_labels[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_radius(date_labels[i], 16, 0);
    lv_obj_set_style_bg_opa(date_labels[i], LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(date_labels[i], color_white, 0);
  }
}
extern uint32_t battery_percent;
void watchface_update(void) {
  struct rtc_time time;
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  if (rtc_time_get(&time) == 0) {
    lv_label_set_text_fmt(label_hour, "%02d", time.tm_hour);
    lv_label_set_text_fmt(label_minute, "%02d", time.tm_min);
    lv_label_set_text_fmt(label_date, "%s %d, %d", months[time.tm_mon], time.tm_mday, time.tm_year + 1900);

    // Set AM/PM icon
    if (time.tm_hour < 12) {
      lv_img_set_src(icon_ampm, &sun);
    } else {
      lv_img_set_src(icon_ampm, &moon);
    }

    // Date row update only
    int wday = time.tm_wday == 0 ? 6 : time.tm_wday - 1;  // tm_wday: 0=Sun, 6=Sat; shift to 0=Mon
    int today = time.tm_mday;
    struct rtc_time t = time;
    int delta = wday;
    t.tm_mday -= delta;
    for (int i = 0; i < 7; ++i) {
      int d = t.tm_mday + i;
      lv_label_set_text_fmt(date_labels[i], "%d", d);
      // Highlight current day with round rect border only, not filled
      if (d == today) {
        lv_obj_set_style_border_color(date_labels[i], color_black, 0);
        lv_obj_set_style_border_width(date_labels[i], 2, 0);
        lv_obj_set_style_radius(date_labels[i], 1, 0);  // Rectangle border
      }

      lv_color_t text_color = (i == 5) ? color_blue : (i == 6) ? color_red : color_black;
      lv_obj_set_style_text_color(date_labels[i], text_color, 0);
    }
  }

  // Update battery status
  int battery_index = battery_percent / 20;
  if (battery_index > 5) {
    battery_index = 5;
  }
  LOG_INF("Battery percent: %u, index: %d", battery_percent, battery_index);
  lv_img_set_src(battery_icon, battery_icons[battery_index]);
  lv_label_set_text_fmt(label_battery_percent, "%u%%", battery_percent);
}
