# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

FILE(GLOB_RECURSE UI_Sources CONFIGURE_DEPENDS src/ui/*.c)

include_directories(
  /Users/phuc/Work/k_watch_fw/modules/lib/gui
  src
  src/hal
)

target_sources(app PRIVATE
  src/main.c
  src/hal/ble.c
  src/hal/led.c
  src/hal/buttons.c
  src/hal/rtc.c
  src/hal/cts_client.c
  src/hal/ancs_client.c
  src/hal/power.c
  src/app.c
  src/app/model.c
  src/app/screens/watchface_screen.c
  src/app/screens/noti_screen.c
  #  src/driver/LPM013M126A.c
  ${LVGL_FONT_SOURCES}
  ${LVGL_IMAGE_SOURCES}
  ${UI_Sources}
)
