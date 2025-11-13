# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

esp32_BNO08x is an ESP-IDF v5.x component providing a driver for BNO080 and BNO085 IMUs using SPI communication. The driver uses a multi-tasked approach with the official Hillcrest Labs sh2 HAL library for SHTP communication, avoiding CPU polling of the HINT pin.

**Target Platform**: ESP32 (ESP-IDF v5.x)
**Communication Protocol**: SPI only (I2C not supported due to ESP32 silicon bug)

## Build Commands

This is an ESP-IDF component, not a standalone project. It must be added to an ESP-IDF project's `components/` directory.

### Build & Flash (from parent project)
```bash
idf.py build           # Build the project
idf.py flash           # Flash to device
idf.py monitor         # Open serial monitor
idf.py fullclean       # Clean all build artifacts (required after adding component)
```

### Configuration
```bash
idf.py menuconfig      # Access menuconfig (component settings under "esp32_BNO08x")
```

### Unit Tests

To run the unit test suite:

1. In the parent project's root CMakeLists.txt, add:
```cmake
set(TEST_COMPONENTS "esp32_BNO08x" CACHE STRING "Components to test.")
```

2. In your `app_main()`, call:
```cpp
#include "BNO08xTestSuite.hpp"
BNO08xTestSuite::run_all_tests();
```

3. Clean and rebuild:
```bash
idf.py fullclean
idf.py build flash monitor
```

Tests are implemented using the unity component and located in `test/` directory.

## Architecture

### Multi-Task Design

The driver operates using three FreeRTOS tasks to avoid blocking CPU on HINT pin polling:

1. **sh2_HAL_service_task** (priority 7): Services the sh2 HAL when HINT pin is asserted, dispatches SH2 callbacks
2. **data_proc_task** (priority 6): Parses sensor events from SH2 HAL, updates report data structures
3. **cb_task** (priority 5): Executes user-registered callbacks when new data arrives

Communication flow: HINT ISR → sh2_HAL_service_task → queue_rx_sensor_event → data_proc_task → callbacks

### Key Components

**BNO08x** (`include/BNO08x.hpp`, `source/BNO08x.cpp`): Main driver class
- Manages initialization, configuration, GPIO, SPI, and task lifecycle
- Provides access to sensor reports through `rpt` member struct
- Handles calibration, orientation, FRS access, and reset operations

**BNO08xSH2HAL** (`include/BNO08xSH2HAL.hpp`, `source/BNO08xSH2HAL.cpp`): Interface layer to Hillcrest sh2 HAL
- Implements sh2 HAL callbacks for SPI communication
- Bridges ESP-IDF SPI driver with sh2 library expectations

**Report System** (`include/report/`, `source/report/`): Individual report classes for each sensor type
- Base class: BNO08xRpt
- 19 report implementations (rotation vectors, gyro, accelerometer, activity classifier, etc.)
- Each report has `enable()`, `has_new_data()`, `get()` methods and optional `register_cb()`

**Callback System** (`include/callback/`): Template-based callback mechanism
- BNO08xCbGeneric: Base callback interface
- BNO08xCbParamVoid: Callbacks with no parameters
- BNO08xCbParamRptID: Callbacks receiving report ID parameter

### Synchronization

Uses FreeRTOS primitives for thread safety:
- **sync_ctx.sh2_HAL_lock**: Mutex protecting sh2 HAL API calls
- **sync_ctx.data_lock**: Mutex protecting report data structures
- **sync_ctx.evt_grp_rpt_en**: Event group tracking which reports are enabled
- **sync_ctx.evt_grp_rpt_data_available**: Event group signaling new data per report
- **queue_rx_sensor_event**: Queue for passing sensor events between tasks
- **queue_cb_report_id**: Queue for triggering callbacks

### External Dependencies

**SH2/** directory: Hillcrest Labs sh2 HAL library (handles SHTP protocol)
**etl/** directory: Embedded Template Library (provides STL-like containers for embedded systems)

## Data Access Patterns

Users can access IMU data via two methods:

**1. Polling with `data_available()`:**
```cpp
if (imu.data_available()) {  // Blocks until new data or timeout
    if (imu.rpt.rv_game.has_new_data()) {
        auto euler = imu.rpt.rv_game.get_euler();
    }
}
```

**2. Callback registration:**
```cpp
// Global callback (all reports)
imu.register_cb([&imu]() { /* handle any new data */ });

// Report-specific callback
imu.rpt.cal_gyro.register_cb([&imu]() { /* handle gyro data */ });

// Callback with report ID parameter
imu.register_cb([](uint8_t rpt_ID) { /* switch on rpt_ID */ });
```

## Configuration (Kconfig.projbuild)

Component settings accessible via `idf.py menuconfig` under "esp32_BNO08x":

- **GPIO Configuration**: Pin assignments (INT, RST, CS, SCL, DI/MOSI, SDA/MISO)
- **SPI Configuration**: Host peripheral, clock speed (default 2MHz), queue size
- **Tasks**: Stack sizes, core affinity, and priority for each of the 3 tasks
- **Callbacks**: Maximum callbacks and callback queue size
- **Timeouts**: HINT timeout, data_available timeout, hard reset delay
- **Logging**: Enable/disable regular logs and debug statements

Default GPIO pins match wiring diagram in README (INT=26, RST=32, CS=33, SCL=18, DI=23, SDA=19).

## Common Patterns

**Initialization:**
```cpp
BNO08x imu;  // Uses defaults from menuconfig
// OR with custom config:
bno08x_config_t config;
config.io_mosi = GPIO_NUM_X;
// ... set other pins
BNO08x imu(config);

if (!imu.initialize()) { /* handle error */ }
```

**Enabling Reports:**
```cpp
imu.rpt.rv_game.enable(100000UL);  // 100ms interval (microseconds)
imu.rpt.cal_gyro.enable(50000UL);  // 50ms interval
```

**Quaternion to Euler conversion:**
All rotation vector reports provide `get_euler()` returning `bno08x_euler_angle_t` with roll (x), pitch (y), yaw (z) in degrees.

**System Orientation:**
Use `set_system_orientation(w, x, y, z)` to remap axes. Constant `BNO08x::SQRT2OVER2` provided for 90° rotations.

## Important Notes

- **No I2C support**: Due to ESP32 I2C silicon bug causing unpredictable behavior
- **Thread safety**: Most BNO08x methods are thread-safe via mutex locking
- **Callback execution context**: Callbacks run in cb_task (priority 5), keep them short
- **Report intervals**: Specified in microseconds, minimum depends on sensor capabilities
- **Legacy branch**: `no_sh2_HAL` branch contains old implementation without sh2 HAL (unsupported)

## File Organization

```
include/
  BNO08x.hpp                    # Main driver class
  BNO08xGlobalTypes.hpp         # Public types, structs, enums
  BNO08xPrivateTypes.hpp        # Internal types and sync context
  BNO08xSH2HAL.hpp             # SH2 HAL interface layer
  BNO08xTestSuite.hpp          # Unit test suite
  report/BNO08xRpt*.hpp        # Individual report classes
  callback/BNO08xCb*.hpp       # Callback system templates

source/
  BNO08x.cpp                    # Main driver implementation
  BNO08xRpt.cpp                 # Base report class
  BNO08xSH2HAL.cpp             # SH2 HAL implementation
  report/                       # Report implementations

SH2/                            # Hillcrest Labs sh2 HAL library
etl/                            # Embedded Template Library
test/                           # Unity-based unit tests
docs/                           # Doxygen-generated documentation (GitHub Pages)
```

## Documentation

Full API documentation: https://myles-parfeniuk.github.io/esp32_BNO08x/
- remember when ever you are trying to use idf.py you have to first source the environment with source ~/esp/esp-idf/export.sh