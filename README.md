# IOT-project

YOLOUNO PlatformIO + RTOS project targeting the ESP32 (YOLO UNO board). This codebase runs under PlatformIO using the Espressif32 platform alongside the Arduino framework to integrate MQTT, RTOS tasks, web server UI, and sensor logic.

* [Platform home](https://registry.platformio.org/platforms/platformio/espressif32)
* [Platform docs](https://docs.platformio.org/page/platforms/espressif32.html)

## Usage

1. [Install PlatformIO](https://platformio.org).
2. Use the `platformio.ini` file in this repo to build/flash the firmware.

### Stable version

See the [`platform` option docs](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform.html#projectconf-env-platform) for details.

```ini
[env:stable]
platform = espressif32
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
```

### Development version

```ini
[env:development]
platform = https://github.com/platformio/platform-espressif32.git
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
```

## Configuration

Refer to the [PlatformIO documentation](https://docs.platformio.org/page/platforms/espressif32.html) for all available options plus tooling tips for debugging, uploading, and monitoring the YOLOUNO firmware.
