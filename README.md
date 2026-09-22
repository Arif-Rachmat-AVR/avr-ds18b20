# **AVR DS18B20 — A DS18B20 Driver**

Lightweight, allocation-free DS18B20 temperature sensor driver for 8-bit AVR microcontrollers. Designed for standard C and C++ bare-metal development workflows and intended for integration with the [AVR-CMake-Template](https://github.com/Arif-Rachmat-AVR/AVR-Cmake-Template).

This library is the **device-specific driver layer** for the DS18B20. It uses the independent [AVR 1-Wire](https://github.com/Arif-Rachmat-AVR/avr-onewire) library for all 1-Wire bus communication, including reset/presence detection, bit and byte transfers, ROM addressing, CRC-8, multidrop search, and timing-critical bus handling.

---

## **Table of Contents 📋**

- [**AVR DS18B20 — A DS18B20 Driver**](#avr-ds18b20--a-ds18b20-driver)
  - [**Table of Contents 📋**](#table-of-contents-)
  - [**Features ✨**](#features-)
  - [**Prerequisites ❗**](#prerequisites-)
  - [**Installation 🛠️**](#installation-️)
    - [**Recommended: Git Submodule**](#recommended-git-submodule)
    - [Why use a submodule?](#why-use-a-submodule)
    - [**Alternative: Git Clone**](#alternative-git-clone)
  - [**Quick Start 🚀**](#quick-start-)
  - [**Usage Example 💡**](#usage-example-)
    - [Using a Known ROM Address](#using-a-known-rom-address)
    - [Single-Device Bus](#single-device-bus)
    - [Parasite Power](#parasite-power)
    - [Preemptive Scheduler Integration](#preemptive-scheduler-integration)
  - [**API Reference 📖**](#api-reference-)
    - [`ds18b20_start_conversion()`](#ds18b20_start_conversion)
    - [`ds18b20_convert_wait()`](#ds18b20_convert_wait)
    - [`ds18b20_read_scratchpad()`](#ds18b20_read_scratchpad)
    - [`ds18b20_read_raw()`](#ds18b20_read_raw)
    - [`ds18b20_raw_to_milli_celsius()`](#ds18b20_raw_to_milli_celsius)
    - [`ds18b20_write_scratchpad()`](#ds18b20_write_scratchpad)
    - [`ds18b20_copy_scratchpad()`](#ds18b20_copy_scratchpad)
    - [`ds18b20_is_parasite_powered()`](#ds18b20_is_parasite_powered)
  - [**Hardware Configuration ⚙️**](#hardware-configuration-️)
    - [Normal VDD Operation](#normal-vdd-operation)
    - [Parasite-Power Operation](#parasite-power-operation)
    - [Bus Connection](#bus-connection)
  - [**Important Notes ⚠️**](#important-notes-️)
    - [Dependency on AVR 1-Wire](#dependency-on-avr-1-wire)
    - [ROM Addressing](#rom-addressing)
    - [Temperature Conversion](#temperature-conversion)
    - [CRC Validation](#crc-validation)
    - [Resolution](#resolution)
    - [Scheduler Compatibility](#scheduler-compatibility)
    - [Error Handling](#error-handling)
  - [**Roadmap 📌**](#roadmap-)
  - [**License 📜**](#license-)

---

## **Features ✨**

* Dedicated DS18B20 device driver built on the generic AVR 1-Wire library.
* DS18B20 family-code validation (`28h`).
* `Convert T` support.
* 9-bit, 10-bit, 11-bit, and 12-bit resolution support.
* Scratchpad read and write operations.
* Scratchpad CRC validation through `avr-onewire`.
* Signed raw temperature access.
* Integer milli-degree Celsius conversion without floating-point requirements.
* ROM-addressed operation using `Match ROM`.
* `Skip ROM` support for single-device operation.
* Parasite-power detection.
* Parasite-power conversion and EEPROM-copy support through the generic strong-pull-up interface.
* No dynamic memory allocation.
* No direct AVR GPIO manipulation inside the device driver.
* No duplicate 1-Wire timing implementation.
* Compatible with the scheduler-aware timing model of `avr-onewire`.
* Designed for both C and C++ AVR projects.
* CMake static-library build.
* No Arduino framework dependency.

---

## **Prerequisites ❗**

>This library requires the independent [AVR 1-Wire](https://github.com/Arif-Rachmat-AVR/avr-onewire) library.

This library is specifically built on top of and designed to integrate with the **[AVR-CMake-Template](https://github.com/Arif-Rachmat/AVR-CMake-Template)** repository. For smooth development and compilation, ensure your project is built using that template as its base.

Your host environment must meet the base project toolchain requirements:

* **Base Project Template**: **[AVR-CMake-Template](https://github.com/Arif-Rachmat/AVR-CMake-Template)** (verify your main application setup follows this structure).
* **AVR Toolchain**: [Microchip AVR Toolchain](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers) (`avr-gcc`, `binutils-avr`, and `avr-libc`).
* **Build System**: [CMake](https://cmake.org/download/) (v3.16+) and a build generator like [Ninja](https://ninja-build.org/) or GNU [Make].
* **Other Requirements** follows the requirements in [AVR 1-Wire](https://github.com/Arif-Rachmat-AVR/avr-onewire) Library.

> **Note**: For platform-specific toolchain installation steps (Windows/MSYS2 UCRT64, Linux/Debian/Ubuntu, or macOS/Homebrew), please refer directly to the **[AVR-CMake-Template Prerequisites](https://github.com/Arif-Rachmat/AVR-CMake-Template#prerequisites)** section.

---

## **Installation 🛠️**

### **Recommended: Git Submodule**

Keep the generic bus library and this device driver as separate dependencies in the application project.

A typical project layout is:

```text
Your-AVR-Project/
├── CMakeLists.txt
├── Lib/
│   ├── onewire/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── onewire.h
│   │   │   └── onewire_config.h
│   │   └── src/
│   │       └── onewire.c
│   └── ds18b20/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── ds18b20.h
│       └── src/
│           └── ds18b20.c
├── src/
│   └── main.cpp
└── ...
```

Add the two repositories independently:

```bash
git submodule add https://github.com/Arif-Rachmat-AVR/avr-onewire.git lib/onewire
git submodule add https://github.com/Arif-Rachmat-AVR/avr-ds18b20.git lib/ds18b20
```

When cloning an existing project containing both libraries:

```bash
git clone --recurse-submodules <your-project-url>
```

If the project has already been cloned without its submodules:

```bash
git submodule update --init --recursive
```

### Why use a submodule?

Using Git submodules keeps the transport and device-driver libraries independently versioned:

* `avr-onewire` can be updated without modifying the DS18B20 driver.
* The DS18B20 driver can pin a known-compatible 1-Wire library revision.
* Other 1-Wire device drivers can use `avr-onewire` independently.
* The application can track exact versions of both dependencies.
* Device-specific functionality remains isolated from the generic protocol layer.

### **Alternative: Git Clone**

For standalone experiments, both repositories can be cloned directly into the project's `lib` directory:

```bash
git clone https://github.com/Arif-Rachmat-AVR/avr-onewire.git lib/onewire
git clone https://github.com/Arif-Rachmat-AVR/avr-ds18b20.git lib/ds18b20
```

The two libraries remain separate CMake targets.

---

## **Quick Start 🚀**

The following example assumes:

* One DS18B20 device.
* DQ connected to `PB0`.
* External pull-up resistor on the 1-Wire bus.
* Normal external VDD power.
* `avr-onewire` has already been added to the project.

```c
#include <avr/io.h>
#include <stdint.h>

#include "onewire.h"
#include "ds18b20.h"

static const onewire_bus_t bus = {
    .data = ONEWIRE_GPIO(DDRB, PORTB, PINB, PB0),
    .strong_pullup = { 0 },
    .strong_pullup_active_high = true,
    .critical = NULL,
};

int main(void)
{
    uint8_t rom[8];
    int16_t raw_temperature;

    if (onewire_init(&bus) != ONEWIRE_OK) {
        for (;;) {
            /* Invalid 1-Wire configuration. */
        }
    }

    if (onewire_read_rom(&bus, rom) != ONEWIRE_OK) {
        for (;;) {
            /* Sensor not detected or ROM CRC failure. */
        }
    }

    if (ds18b20_convert_wait(&bus,
                             rom,
                             false,
                             DS18B20_RESOLUTION_12BIT) != ONEWIRE_OK) {
        for (;;) {
            /* Temperature conversion failed. */
        }
    }

    if (ds18b20_read_raw(&bus, rom, &raw_temperature) != ONEWIRE_OK) {
        for (;;) {
            /* Scratchpad read or CRC failure. */
        }
    }

    int32_t temperature_mC =
        ds18b20_raw_to_milli_celsius(raw_temperature);

    (void)temperature_mC;

    for (;;) {
        /* Main application. */
    }
}
```

The application uses `avr-onewire` to configure and communicate with the bus, while `avr-ds18b20` handles only DS18B20-specific commands and data interpretation.

---

## **Usage Example 💡**

### Using a Known ROM Address

For a multidrop bus, first discover the devices with `avr-onewire`:

```c
onewire_search_t search;
onewire_search_init(&search);

while (onewire_search_next(&bus, &search) == ONEWIRE_OK) {
    if (search.rom[0] == DS18B20_FAMILY_CODE) {
        /* search.rom contains one DS18B20 ROM address. */
    }
}
```

The discovered ROM can then be passed to the DS18B20 driver:

```c
uint8_t rom[8];

int16_t raw_temperature;

if (ds18b20_convert_wait(&bus,
                         rom,
                         false,
                         DS18B20_RESOLUTION_12BIT) == ONEWIRE_OK) {

    if (ds18b20_read_raw(&bus,
                         rom,
                         &raw_temperature) == ONEWIRE_OK) {

        int32_t temperature_mC =
            ds18b20_raw_to_milli_celsius(raw_temperature);

        (void)temperature_mC;
    }
}
```

The ROM search itself belongs to `avr-onewire`; this driver only interprets the selected ROM as a DS18B20 address.

### Single-Device Bus

For a single-device bus, the driver allows `rom == NULL` for operations that use `Skip ROM`:

```c
int16_t raw_temperature;

if (ds18b20_convert_wait(&bus,
                         NULL,
                         false,
                         DS18B20_RESOLUTION_12BIT) == ONEWIRE_OK) {

    ds18b20_read_raw(&bus,
                     NULL,
                     &raw_temperature);
}
```

`Skip ROM` should only be used when the application knows that multiple devices will not respond simultaneously to the selected device command.

### Parasite Power

When the DS18B20 uses parasite power, configure an external strong-pull-up stage:

```c
static const onewire_bus_t bus = {
    .data = ONEWIRE_GPIO(DDRB, PORTB, PINB, PB0),
    .strong_pullup = ONEWIRE_OUTPUT_GPIO(DDRB, PORTB, PB1),
    .strong_pullup_active_high = true,
    .critical = NULL,
};
```

Then enable parasite-powered operation:

```c
ds18b20_convert_wait(&bus,
                     rom,
                     true,
                     DS18B20_RESOLUTION_12BIT);
```

The DS18B20 driver uses the generic `avr-onewire` strong-pull-up mechanism so that the command byte and pull-up assertion are handled as one timing-sensitive operation.

The strong-pull-up GPIO should control an appropriate external transistor, MOSFET, or power-switching circuit. The AVR GPIO should not be used as the sensor's high-current supply path.

### Preemptive Scheduler Integration

Scheduler integration is inherited from `avr-onewire`.

The DS18B20 driver does not independently disable interrupts. Timing-sensitive 1-Wire slots are protected by the generic bus layer, while longer sensor operations do not require the entire CPU to remain in a critical section.

Configure the scheduler hook in the `onewire_bus_t`:

```c
static const onewire_critical_ops_t scheduler_critical = {
    .enter = scheduler_enter,
    .exit = scheduler_exit,
    .context = NULL,
};

static const onewire_bus_t bus = {
    .data = ONEWIRE_GPIO(DDRB, PORTB, PINB, PB0),
    .strong_pullup = { 0 },
    .strong_pullup_active_high = true,
    .critical = &scheduler_critical,
};
```

The DS18B20 driver automatically uses the configured bus behavior because all bus communication is delegated to `avr-onewire`.

---

## **API Reference 📖**

### `ds18b20_start_conversion()`

Starts a DS18B20 temperature conversion.

```c
onewire_status_t ds18b20_start_conversion(const onewire_bus_t *bus,
                                          const uint8_t rom[8],
                                          bool parasite_power);
```

Behavior:

* `rom == NULL` uses `Skip ROM`.
* An explicit ROM uses `Match ROM`.
* An explicit ROM must contain the DS18B20 family code `28h`.
* `parasite_power == true` requires a configured strong-pull-up stage.

### `ds18b20_convert_wait()`

Starts a conversion and waits for the maximum conversion time associated with the selected resolution.

```c
ds18b20_convert_wait(&bus,
                     rom,
                     false,
                     DS18B20_RESOLUTION_12BIT);
```

Supported resolutions:

```c
DS18B20_RESOLUTION_9BIT
DS18B20_RESOLUTION_10BIT
DS18B20_RESOLUTION_11BIT
DS18B20_RESOLUTION_12BIT
```

### `ds18b20_read_scratchpad()`

Reads and CRC-checks the complete nine-byte DS18B20 scratchpad.

```c
ds18b20_scratchpad_t scratchpad;

onewire_status_t status =
    ds18b20_read_scratchpad(&bus,
                            rom,
                            &scratchpad);
```

The returned structure contains:

* Raw temperature LSB/MSB.
* `TH`.
* `TL`.
* Configuration register.
* Reserved bytes.
* CRC byte.

### `ds18b20_read_raw()`

Reads the signed 16-bit temperature register.

```c
int16_t raw_temperature;

ds18b20_read_raw(&bus,
                  rom,
                  &raw_temperature);
```

At 12-bit resolution, the raw representation is in units of 1/16 °C.

### `ds18b20_raw_to_milli_celsius()`

Converts a raw DS18B20 temperature value to integer milli-degrees Celsius.

```c
int32_t temperature_mC =
    ds18b20_raw_to_milli_celsius(raw_temperature);
```

No floating-point arithmetic is required.

### `ds18b20_write_scratchpad()`

Writes the DS18B20 alarm thresholds and resolution configuration.

```c
ds18b20_write_scratchpad(&bus,
                         rom,
                         30,
                         10,
                         DS18B20_RESOLUTION_12BIT);
```

The function writes:

```text
TH
TL
Configuration
```

to the device scratchpad.

### `ds18b20_copy_scratchpad()`

Copies the scratchpad configuration into the DS18B20 EEPROM.

```c
ds18b20_copy_scratchpad(&bus,
                        rom,
                        false);
```

Use `true` for parasite-powered operation when the configured strong-pull-up hardware is required.

### `ds18b20_is_parasite_powered()`

Reads the device's power-supply mode.

```c
bool parasite_powered;

ds18b20_is_parasite_powered(&bus,
                            rom,
                            &parasite_powered);
```

---

## **Hardware Configuration ⚙️**

### Normal VDD Operation

A typical normal-powered DS18B20 connection is:

![Conventional One-Wire connection](/img/ExternalSupply.png)

The DS18B20 receives power through its VDD pin while DQ is used for 1-Wire communication.

### Parasite-Power Operation

In parasite-power mode, the DS18B20 obtains power through the 1-Wire bus. A strong-pull-up circuit should therefore be provided for operations that require additional current:

![Parasite-Powered One-Wire connection](/img/ParasitePower.png)

The actual circuit depends on the supply voltage, device requirements, transistor/MOSFET selection, and bus design.

### Bus Connection

The DS18B20 driver does not directly configure AVR registers. The complete bus configuration is supplied through `onewire_bus_t` from the generic `avr-onewire` library:

```c
static const onewire_bus_t bus = {
    .data = ONEWIRE_GPIO(DDRB, PORTB, PINB, PB0),
    .strong_pullup = { 0 },
    .strong_pullup_active_high = true,
    .critical = NULL,
};
```

This keeps the DS18B20 driver independent from AVR-family-specific GPIO implementations.

---

## **Important Notes ⚠️**

### Dependency on AVR 1-Wire

`avr-ds18b20` is a device driver, not a standalone 1-Wire implementation.

Its dependency chain is:

```text
avr-ds18b20
     │
     └── avr-onewire
              │
              └── AVR GPIO / timing
```

Do not duplicate 1-Wire timing code inside an application using this driver.

### ROM Addressing

A DS18B20 has a unique 64-bit ROM address.

The generic `avr-onewire` library is responsible for:

* Reading ROM codes.
* Searching a multidrop bus.
* Performing ROM CRC validation.
* Selecting devices with `Match ROM`.

This driver only uses those ROM addresses when issuing DS18B20-specific commands.

For an explicit address, the first ROM byte must be `DS18B20_FAMILY_CODE`, which is `0x28`.

### Temperature Conversion

The DS18B20 performs the actual temperature conversion internally after receiving `Convert T`. The driver does not need to keep the 1-Wire bus active during the sensor's internal conversion period. `ds18b20_convert_wait()` simply waits for the maximum conversion time associated with the requested resolution while the application's normal interrupt behavior remains available.

### CRC Validation

The DS18B20 scratchpad contains a CRC byte. The driver validates the scratchpad using the CRC implementation provided by `avr-onewire`. Therefore a corrupted scratchpad is reported through `ONEWIRE_ERR_CRC`, rather than silently returning an invalid temperature.

### Resolution

The driver supports:

| Resolution | Maximum conversion time |
| ---------- | ----------------------: |
| 9-bit      |                   94 ms |
| 10-bit     |                  188 ms |
| 11-bit     |                  375 ms |
| 12-bit     |                  750 ms |

The driver waits for these maximum conversion periods when using `ds18b20_convert_wait()`.

### Scheduler Compatibility

The DS18B20 driver does not own the global interrupt state.

All 1-Wire timing-sensitive operations pass through `avr-onewire`, which provides the configurable critical-section mechanism.

For a preemptive scheduler, configure the critical-section hooks at the `onewire_bus_t` level rather than modifying this DS18B20 driver.

### Error Handling

The driver uses the `onewire_status_t` type supplied by `avr-onewire`:

| Status                         | Meaning                                            |
| ------------------------------ | -------------------------------------------------- |
| `ONEWIRE_OK`                   | Operation completed successfully                   |
| `ONEWIRE_ERR_INVALID_ARG`      | Invalid pointer, ROM, resolution, or configuration |
| `ONEWIRE_ERR_NO_DEVICE`        | Device did not respond to reset                    |
| `ONEWIRE_ERR_CRC`              | CRC validation failed                              |
| `ONEWIRE_ERR_SEARCH_END`       | No more devices during ROM search                  |
| `ONEWIRE_ERR_NO_STRONG_PULLUP` | Required strong-pull-up hardware is not configured |

---

## **Roadmap 📌**

* [ ] Add optional asynchronous/non-blocking conversion API.
* [ ] Add a direct `read_temperature_mC()` convenience API.
* [ ] Add dedicated alarm-threshold helper functions.
* [ ] Add optional scratchpad caching.
* [ ] Expand hardware-in-the-loop test coverage.
* [ ] Add dedicated multidrop DS18B20 examples.
* [ ] Keep the DS18B20 implementation independently versioned from `avr-onewire`.

---

## **License 📜**

This project is licensed under the [MIT License](LICENSE) — free for both personal and commercial use.
