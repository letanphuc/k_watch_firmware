/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ancs_client.h"

#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/gattp.h>
#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include "app.h"

LOG_MODULE_REGISTER(app_ancs_client);

enum { DISCOVERY_ANCS_ONGOING, DISCOVERY_ANCS_SUCCEEDED, SERVICE_CHANGED_INDICATED };

static struct bt_ancs_client ancs_c;
static struct bt_gattp gattp;
static atomic_t discovery_flags;

/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;
/* Local copy of the newest notification attribute. */
static struct bt_ancs_attr notif_attr_latest;
typedef struct {
  uint8_t app_id[ATTR_APP_ID_SIZE];
  uint8_t title[ATTR_TITLE_SIZE];
  uint8_t subtitle[ATTR_COMMON_SIZE];
  uint8_t message[ATTR_MESSAGE_SIZE];
  uint8_t message_size[ATTR_COMMON_SIZE];
  uint8_t date[ATTR_COMMON_SIZE];
  uint8_t posaction[ATTR_COMMON_SIZE];
  uint8_t negaction[ATTR_COMMON_SIZE];
  uint8_t disp_name[ATTR_COMMON_SIZE];
} ancs_attr_buffers_t;

static ancs_attr_buffers_t attr_bufs;

/* String literals for the iOS notification categories. */
static const char* lit_catid[BT_ANCS_CATEGORY_ID_COUNT] = {
    "Other", "Incoming Call", "Missed Call",        "Voice Mail",           "Social",   "Schedule",
    "Email", "News",          "Health And Fitness", "Business And Finance", "Location", "Entertainment"};

/* String literals for the iOS notification event types. */
static const char* lit_eventid[BT_ANCS_EVT_ID_COUNT] = {"Added", "Modified", "Removed"};

/* String literals for the iOS notification attribute types. */
static const char* lit_attrid[BT_ANCS_NOTIF_ATTR_COUNT] = {
    "App Identifier",       "Title", "Subtitle", "Message", "Message Size", "Date", "Positive Action Label",
    "Negative Action Label"};

/* String literals for the iOS notification attribute types. */
static const char* lit_appid[BT_ANCS_APP_ATTR_COUNT] = {"Display Name"};

static void discover_ancs_first(struct bt_conn* conn);
static void discover_ancs_again(struct bt_conn* conn);
static void bt_ancs_notification_source_handler(struct bt_ancs_client* ancs_c, int err,
                                                const struct bt_ancs_evt_notif* notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client* ancs_c, const struct bt_ancs_attr_response* response);

static void enable_ancs_notifications(struct bt_ancs_client* ancs_c) {
  int err;

  err = bt_ancs_subscribe_notification_source(ancs_c, bt_ancs_notification_source_handler);
  if (err) {
    LOG_ERR("Failed to enable Notification Source notification (err %d)", err);
  }

  err = bt_ancs_subscribe_data_source(ancs_c, bt_ancs_data_source_handler);
  if (err) {
    LOG_ERR("Failed to enable Data Source notification (err %d)", err);
  }
}

static void discover_ancs_completed_cb(struct bt_gatt_dm* dm, void* ctx) {
  int err;
  struct bt_ancs_client* ancs_c = (struct bt_ancs_client*)ctx;
  struct bt_conn* conn = bt_gatt_dm_conn_get(dm);

  LOG_INF("The discovery procedure for ANCS succeeded");

  bt_gatt_dm_data_print(dm);

  err = bt_ancs_handles_assign(dm, ancs_c);
  if (err) {
    LOG_ERR("Could not init ANCS client object, error: %d", err);
  } else {
    atomic_set_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED);
    enable_ancs_notifications(ancs_c);
  }

  err = bt_gatt_dm_data_release(dm);
  if (err) {
    LOG_ERR("Could not release the discovery data, error code: %d", err);
  }

  atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
  discover_ancs_again(conn);
}

static void discover_ancs_not_found_cb(struct bt_conn* conn, void* ctx) {
  LOG_WRN("ANCS could not be found during the discovery");

  atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
  discover_ancs_again(conn);
}

static void discover_ancs_error_found_cb(struct bt_conn* conn, int err, void* ctx) {
  LOG_ERR("The discovery procedure for ANCS failed, err %d", err);

  atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
  discover_ancs_again(conn);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
    .completed = discover_ancs_completed_cb,
    .service_not_found = discover_ancs_not_found_cb,
    .error_found = discover_ancs_error_found_cb,
};

static void indicate_sc_cb(struct bt_gattp* gattp, const struct bt_gattp_handle_range* handle_range, int err) {
  if (!err) {
    atomic_set_bit(&discovery_flags, SERVICE_CHANGED_INDICATED);
    discover_ancs_again(gattp->conn);
  }
}

static void enable_gattp_indications(struct bt_gattp* gattp) {
  int err;

  err = bt_gattp_subscribe_service_changed(gattp, indicate_sc_cb);
  if (err) {
    LOG_ERR("Cannot subscribe to Service Changed indication (err %d)", err);
  }
}

static void discover_gattp_completed_cb(struct bt_gatt_dm* dm, void* ctx) {
  int err;
  struct bt_gattp* gattp = (struct bt_gattp*)ctx;
  struct bt_conn* conn = bt_gatt_dm_conn_get(dm);

  if (bt_gatt_dm_attr_cnt(dm) > 1) {
    LOG_INF("The discovery procedure for GATT Service succeeded");

    bt_gatt_dm_data_print(dm);

    err = bt_gattp_handles_assign(dm, gattp);
    if (err) {
      LOG_ERR("Could not init GATT Service client object, error: %d", err);
    } else {
      enable_gattp_indications(gattp);
    }
  } else {
    LOG_WRN("GATT Service could not be found during the discovery");
  }

  err = bt_gatt_dm_data_release(dm);
  if (err) {
    LOG_ERR("Could not release the discovery data, error code: %d", err);
  }

  discover_ancs_first(conn);
}

static void discover_gattp_not_found_cb(struct bt_conn* conn, void* ctx) {
  LOG_WRN("GATT Service could not be found during the discovery");

  discover_ancs_first(conn);
}

static void discover_gattp_error_found_cb(struct bt_conn* conn, int err, void* ctx) {
  LOG_ERR("The discovery procedure for GATT Service failed, err %d", err);

  discover_ancs_first(conn);
}

static const struct bt_gatt_dm_cb discover_gattp_cb = {
    .completed = discover_gattp_completed_cb,
    .service_not_found = discover_gattp_not_found_cb,
    .error_found = discover_gattp_error_found_cb,
};

static void discover_gattp(struct bt_conn* conn) {
  int err;

  err = bt_gatt_dm_start(conn, BT_UUID_GATT, &discover_gattp_cb, &gattp);
  if (err) {
    LOG_ERR("Failed to start discovery for GATT Service (err %d)", err);
  }
}

static void discover_ancs(struct bt_conn* conn, bool retry) {
  int err;

  /* 1 Service Discovery at a time */
  if (atomic_test_and_set_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING)) {
    return;
  }

  /* If ANCS is found, do not discover again. */
  if (atomic_test_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED)) {
    atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
    return;
  }

  /* Check that Service Changed indication is received before discovering ANCS again. */
  if (retry) {
    if (!atomic_test_and_clear_bit(&discovery_flags, SERVICE_CHANGED_INDICATED)) {
      atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
      return;
    }
  }

  err = bt_gatt_dm_start(conn, BT_UUID_ANCS, &discover_ancs_cb, &ancs_c);
  if (err) {
    LOG_ERR("Failed to start discovery for ANCS (err %d)", err);
    atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
  }
}

static void discover_ancs_first(struct bt_conn* conn) { discover_ancs(conn, false); }

static void discover_ancs_again(struct bt_conn* conn) { discover_ancs(conn, true); }

static void security_changed(struct bt_conn* conn, bt_security_t level, enum bt_security_err err) {
  if (!err) {
    if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
      discovery_flags = ATOMIC_INIT(0);
      discover_gattp(conn);
    }
  }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .security_changed = security_changed,
};

static void notif_print(const struct bt_ancs_evt_notif* notif) {
  LOG_INF("[%u] - %s: %s", (unsigned int)notif->notif_uid, lit_eventid[notif->evt_id], lit_catid[notif->category_id]);
}

static void notif_attr_print(const struct bt_ancs_attr* attr) {
  if (attr->attr_len != 0) {
    LOG_INF("%s: %s", lit_attrid[attr->attr_id], (char*)attr->attr_data);
  } else if (attr->attr_len == 0) {
    LOG_INF("%s: (N/A)", lit_attrid[attr->attr_id]);
  }
}

static void app_attr_print(const struct bt_ancs_attr* attr) {
  if (attr->attr_len != 0) {
    LOG_INF("%s: %s", lit_appid[attr->attr_id], (char*)attr->attr_data);
  } else if (attr->attr_len == 0) {
    LOG_INF("%s: (N/A)", lit_appid[attr->attr_id]);
  }
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client* ancs_c, int err,
                                                const struct bt_ancs_evt_notif* notif) {
  if (!err && notif->evt_id == BT_ANCS_EVENT_ID_NOTIFICATION_ADDED) {
    notification_latest = *notif;
    notif_print(&notification_latest);

    // Request notification attributes (content) for the new notification
    int req_err = bt_ancs_request_attrs(ancs_c, notif, NULL);
    if (req_err) {
      LOG_ERR("Failed to request notification attributes (err %d)\n", req_err);
    }
    // Do not emit any event here
  }
}

void ancs_noti_info_free(ancs_noti_info_t* noti) {
  if (noti == NULL) {
    return;
  }
  if (noti->title) {
    k_free(noti->title);
  }
  if (noti->message) {
    k_free(noti->message);
  }
  if (noti->app) {
    k_free(noti->app);
  }
  k_free(noti);
}

static void bt_ancs_data_source_handler(struct bt_ancs_client* ancs_c, const struct bt_ancs_attr_response* response) {
  static ancs_noti_info_t noti_info;
  switch (response->command_id) {
    case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES: {
      notif_attr_latest = response->attr;
      notif_attr_print(&notif_attr_latest);

      uint16_t len = response->attr.attr_len;
      char* str = NULL;

      if (len > 0) {
        str = k_malloc(len + 1);
        if (str) {
          memcpy(str, response->attr.attr_data, len);
          str[len] = '\0';
        }
      }

      if (response->attr.attr_id == BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
        if (noti_info.app) k_free(noti_info.app);
        noti_info.app = str;
      } else if (response->attr.attr_id == BT_ANCS_NOTIF_ATTR_ID_TITLE) {
        if (noti_info.title) k_free(noti_info.title);
        noti_info.title = str;
      } else if (response->attr.attr_id == BT_ANCS_NOTIF_ATTR_ID_MESSAGE) {
        if (noti_info.message) k_free(noti_info.message);
        if (str && len > 256) {
          str[256] = '\0';
        }
        noti_info.message = str;
      } else {
        if (str) k_free(str);
      }

      // Post event after all three important fields are received
      if (noti_info.title && noti_info.message && noti_info.app) {
        ancs_noti_info_t* info_ptr = k_malloc(sizeof(ancs_noti_info_t));
        if (info_ptr) {
          *info_ptr = noti_info;
          app_event_t event = {
              .type = APP_EVENT_BLE_ANCS,
              .ptr = info_ptr,
              .len = sizeof(ancs_noti_info_t),
          };
          app_event_post(&event);
        } else {
          LOG_ERR("Failed to allocate memory for ANCS notification info\n");
          if (noti_info.title) k_free(noti_info.title);
          if (noti_info.message) k_free(noti_info.message);
          if (noti_info.app) k_free(noti_info.app);
        }
        memset(&noti_info, 0, sizeof(noti_info));
      }
      break;
    }

    case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
      app_attr_print(&response->attr);
      break;

    default:
      /* No implementation needed. */
      break;
  }
}

static int ancs_c_init(void) {
  int err;

  err = bt_ancs_client_init(&ancs_c);
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER, &attr_bufs.app_id[0],
                              sizeof(attr_bufs.app_id));
  if (err) {
    return err;
  }

  err = bt_ancs_register_app_attr(&ancs_c, BT_ANCS_APP_ATTR_ID_DISPLAY_NAME, attr_bufs.disp_name,
                                  sizeof(attr_bufs.disp_name));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_TITLE, attr_bufs.title, sizeof(attr_bufs.title));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_MESSAGE, attr_bufs.message, sizeof(attr_bufs.message));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_SUBTITLE, attr_bufs.subtitle, sizeof(attr_bufs.subtitle));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE, attr_bufs.message_size,
                              sizeof(attr_bufs.message_size));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_DATE, attr_bufs.date, sizeof(attr_bufs.date));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL, attr_bufs.posaction,
                              sizeof(attr_bufs.posaction));
  if (err) {
    return err;
  }

  err = bt_ancs_register_attr(&ancs_c, BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL, attr_bufs.negaction,
                              sizeof(attr_bufs.negaction));

  return err;
}

static int gattp_init(void) { return bt_gattp_init(&gattp); }

int ancs_client_init(void) {
  int err;

  err = ancs_c_init();
  if (err) {
    LOG_ERR("ANCS client init failed (err %d)", err);
    return err;
  }

  err = gattp_init();
  if (err) {
    LOG_ERR("GATT Service client init failed (err %d)", err);
    return err;
  }

  return 0;
}
