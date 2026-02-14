
# k-watch Firmware

Firmware for k-watch, an open-source smartwatch.

## Overview
- Based on Zephyr RTOS, Nordic NRF SDK, and SquareLine Studio for UI.
- Modular structure: main app, drivers, board support, UI, tests.

## Coding Guidelines
- Follow Google C/C++ style.
- Use clang-format with the config at `k_watch_firmware/.clang-format`.
- Keep code modular, readable, and well-commented.

## Key Structure
- `k_watch_firmware/app/` – Main app logic, UI, drivers, board support.
- `zephyr/`, `nrf/`, `nrfxlib/` – Upstream RTOS, SDK, and libraries.
- `bootloader/` – Bootloader sources.
- `tools/` – Utilities and scripts.

## Get source
The first step is to initialize the workspace folder (``ws``) where
the ``ws`` and all nRF Connect SDK modules will be cloned.
Run the following commands:

```shell
# Initialize workspace for the k-watch firmware
west init -m https://github.com/letanphuc/k_watch_firmware ws
cd ws
west update
```

## Build & Flash
- Build: `west build -b k_watch k_watch_firmware/app`
- Flash: `west flash --runner jlink`

## Contribution
- All contributions must comply with the open-source license.
- See this README and project files for more details.


## Original project
Original project is at [K-Watch](https://github.com/SonDinh23/K-Watch)