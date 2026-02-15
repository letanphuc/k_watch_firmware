#Copyright(c) 2024 Nordic Semiconductor ASA
#SPDX - License - Identifier : Apache - 2.0

zephyr_library()

    zephyr_library_sources_ifdef(CONFIG_DRV2603 drv2603.c)
