# K-Watch Firmware Architecture

## 1. UI Layer
- UI code is auto-generated in `app/src/ui` (do not modify).
- `app.c` acts as a wrapper, handling UI updates and event processing.

## 2. Event Broker
- Central Zephyr `k_msgq` (message queue) for all events.
- Event struct:
  - `event_id` (type of event)
  - `union { uint32_t u32; void* ptr; }` for data
  - Optional `len` for pointer data
- All events (button, RTC, BLE, notifications) use this struct.

## 3. HAL Layer
- Each HAL module (button, rtc, ble) posts events to the queue.
- For small data: use `u32` in the event.
- For large data: allocate buffer, set `ptr` + `len`, post event.

## 4. App/Event Loop
- App pulls events from the queue and dispatches to handlers.
- If event uses `ptr`, app is responsible for freeing memory after processing.

## 5. Decoupling
- HAL and UI/app are decoupled via the event queue.
- No direct calls between HAL and UI.

## Benefits
- Modular, memory-safe, and scalable for both short and long event data.
- Clean separation of concerns for maintainability and testing.
