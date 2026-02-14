#include "cts_client.h"

#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/cts_client.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "rtc.h"

LOG_MODULE_REGISTER(app_cts_client);

static struct bt_cts_client cts_c;
static bool has_cts;

static bool cts_to_rtc_time(const struct bt_cts_current_time *cts, struct rtc_time *out) {
  uint8_t dow;

  if ((cts->exact_time_256.year == 0) || (cts->exact_time_256.month == 0) || (cts->exact_time_256.day == 0)) {
    return false;
  }

  out->tm_year = cts->exact_time_256.year - 1900;
  out->tm_mon = cts->exact_time_256.month - 1;
  out->tm_mday = cts->exact_time_256.day;
  out->tm_hour = cts->exact_time_256.hours;
  out->tm_min = cts->exact_time_256.minutes;
  out->tm_sec = cts->exact_time_256.seconds;

  dow = cts->exact_time_256.day_of_week;
  if ((dow >= 1) && (dow <= 7)) {
    out->tm_wday = dow % 7;
  } else {
    out->tm_wday = 0;
  }

  return true;
}

static void apply_current_time(const struct bt_cts_current_time *current_time) {
  struct rtc_time rtc;
  int err;

  if (!cts_to_rtc_time(current_time, &rtc)) {
    LOG_WRN("CTS time invalid");
    return;
  }

  err = rtc_time_set(&rtc);
  if (err) {
    LOG_ERR("RTC set failed: %d", err);
    return;
  }

  LOG_INF("RTC synced: %04d-%02d-%02d %02d:%02d:%02d", rtc.tm_year + 1900, rtc.tm_mon + 1, rtc.tm_mday, rtc.tm_hour,
          rtc.tm_min, rtc.tm_sec);
}

static void notify_current_time_cb(struct bt_cts_client *cts, struct bt_cts_current_time *current_time) {
  ARG_UNUSED(cts);
  apply_current_time(current_time);
}

static void read_current_time_cb(struct bt_cts_client *cts, struct bt_cts_current_time *current_time, int err) {
  ARG_UNUSED(cts);

  if (err) {
    LOG_ERR("CTS read failed: %d", err);
    return;
  }

  apply_current_time(current_time);
}

static void enable_notifications(void) {
  int err;

  if (!has_cts || (bt_conn_get_security(cts_c.conn) < BT_SECURITY_L2)) {
    return;
  }

  err = bt_cts_subscribe_current_time(&cts_c, notify_current_time_cb);
  if (err) {
    LOG_ERR("CTS notify subscribe failed: %d", err);
    return;
  }

  err = bt_cts_read_current_time(&cts_c, read_current_time_cb);
  if (err) {
    LOG_ERR("CTS initial read failed: %d", err);
  }
}

static void discover_completed_cb(struct bt_gatt_dm *dm, void *ctx) {
  int err;

  ARG_UNUSED(ctx);

  LOG_INF("CTS discovery complete");

  err = bt_cts_handles_assign(dm, &cts_c);
  if (err) {
    LOG_ERR("CTS handle assign failed: %d", err);
  } else {
    has_cts = true;

    if (bt_conn_get_security(cts_c.conn) < BT_SECURITY_L2) {
      err = bt_conn_set_security(cts_c.conn, BT_SECURITY_L2);
      if (err) {
        LOG_ERR("Set security failed: %d", err);
      }
    } else {
      enable_notifications();
    }
  }

  err = bt_gatt_dm_data_release(dm);
  if (err) {
    LOG_ERR("CTS discovery release failed: %d", err);
  }
}

static void discover_service_not_found_cb(struct bt_conn *conn, void *ctx) {
  ARG_UNUSED(conn);
  ARG_UNUSED(ctx);

  LOG_WRN("CTS not found");
}

static void discover_error_found_cb(struct bt_conn *conn, int err, void *ctx) {
  ARG_UNUSED(conn);
  ARG_UNUSED(ctx);

  LOG_ERR("CTS discovery failed: %d", err);
}

static const struct bt_gatt_dm_cb discover_cb = {
    .completed = discover_completed_cb,
    .service_not_found = discover_service_not_found_cb,
    .error_found = discover_error_found_cb,
};

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    return;
  }

  has_cts = false;

  err = bt_gatt_dm_start(conn, BT_UUID_CTS, &discover_cb, NULL);
  if (err) {
    LOG_ERR("CTS discovery start failed: %d", err);
  }
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
  if (!err) {
    enable_notifications();
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .security_changed = security_changed,
};

int cts_client_init(void) {
  int err;

  err = bt_cts_client_init(&cts_c);
  if (err) {
    LOG_ERR("CTS client init failed: %d", err);
    return err;
  }

  return 0;
}
