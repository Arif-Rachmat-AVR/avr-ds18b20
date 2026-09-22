/**
 * @file ds18b20.h
 * @brief DS18B20 digital temperature sensor driver for AVR microcontrollers.
 *
 * @details Provides a lightweight device-level driver for the Maxim/Analog Devices
 *          DS18B20 1-Wire digital thermometer.
 *
 *          This library is built on top of the generic `avr-onewire` library and
 *          provides DS18B20-specific functionality without owning or abstracting
 *          the underlying 1-Wire bus implementation.
 *
 *          The library provides:
 *          - DS18B20 temperature conversion.
 *          - Configurable 9-bit to 12-bit temperature resolution.
 *          - Scratchpad reading with CRC validation.
 *          - Raw temperature reading without floating-point arithmetic.
 *          - Integer-only conversion to milli-degrees Celsius.
 *          - TH/TL alarm threshold configuration.
 *          - Scratchpad-to-EEPROM copying.
 *          - Parasite-power mode detection.
 *          - Support for both individually addressed and broadcast operations.
 *
 *          Device selection is performed using the 64-bit ROM code provided by
 *          the underlying 1-Wire bus library.
 *
 * @author Arif Rachmat (ngaripar1203@gmail.com)
 * @date 2026-09-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2026 Muhammad Arif Rachmat
 *            Licensed under the MIT License (see LICENSE for details).
 *
 * @note This library depends on the `avr-onewire` library for all 1-Wire bus
 *       communication, timing, ROM addressing, and CRC primitives.
 *
 * @warning Parasite-powered DS18B20 devices require a suitable strong pull-up
 *          on the 1-Wire bus during temperature conversion and EEPROM operations.
 *          The hardware used for strong pull-up must be configured through the
 *          underlying `onewire_bus_t` interface.
 */

#ifndef AVR_DS18B20_H
#define AVR_DS18B20_H

#include <stdbool.h>
#include <stdint.h>

#include "onewire.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DS18B20 device family code.
 *
 * The family code is the first byte of the device's 64-bit ROM identifier.
 */
#define DS18B20_FAMILY_CODE              0x28u

/**
 * @brief DS18B20 "Convert T" command.
 *
 * Starts a temperature conversion.
 */
#define DS18B20_CMD_CONVERT_T            0x44u

/**
 * @brief DS18B20 "Write Scratchpad" command.
 *
 * Writes the TH, TL, and configuration registers.
 */
#define DS18B20_CMD_WRITE_SCRATCHPAD     0x4Eu

/**
 * @brief DS18B20 "Read Scratchpad" command.
 *
 * Reads the complete 9-byte scratchpad including the CRC byte.
 */
#define DS18B20_CMD_READ_SCRATCHPAD      0xBEu

/**
 * @brief DS18B20 "Copy Scratchpad" command.
 *
 * Copies TH, TL, and configuration registers from the scratchpad to
 * non-volatile EEPROM.
 */
#define DS18B20_CMD_COPY_SCRATCHPAD      0x48u

/**
 * @brief DS18B20 "Recall E2" command.
 *
 * Recalls TH, TL, and configuration values from EEPROM into the
 * scratchpad.
 */
#define DS18B20_CMD_RECALL_E2            0xB8u

/**
 * @brief DS18B20 "Read Power Supply" command.
 *
 * Determines whether the device is using parasite power or an external
 * power supply.
 */
#define DS18B20_CMD_READ_POWER_SUPPLY    0xB4u

/**
 * @brief Size of the DS18B20 scratchpad in bytes.
 *
 * The scratchpad consists of eight data bytes followed by one CRC byte.
 */
#define DS18B20_SCRATCHPAD_SIZE          9u

/**
 * @brief DS18B20 temperature resolution.
 *
 * The resolution determines the temperature conversion time and the
 * number of valid fractional temperature bits returned by the device.
 *
 * Higher resolutions provide finer temperature measurements but require
 * longer conversion times.
 */
typedef enum {
    DS18B20_RESOLUTION_9BIT  = 9u,  /**< 0.5 °C resolution. */
    DS18B20_RESOLUTION_10BIT = 10u, /**< 0.25 °C resolution. */
    DS18B20_RESOLUTION_11BIT = 11u, /**< 0.125 °C resolution. */
    DS18B20_RESOLUTION_12BIT = 12u  /**< 0.0625 °C resolution. */
} ds18b20_resolution_t;

/**
 * @brief Raw DS18B20 scratchpad contents.
 *
 * Represents the complete 9-byte DS18B20 scratchpad in the order specified
 * by the device datasheet.
 *
 * The temperature value is stored as a signed 16-bit value formed from
 * `temperature_msb` and `temperature_lsb`. The final byte contains the
 * Dallas/Maxim CRC for the preceding eight bytes.
 *
 * @note The reserved bytes should not be modified by the application.
 */
typedef struct {
    uint8_t temperature_lsb; /**< Temperature register least-significant byte. */
    uint8_t temperature_msb; /**< Temperature register most-significant byte. */
    int8_t  th;              /**< Temperature alarm high threshold. */
    int8_t  tl;              /**< Temperature alarm low threshold. */
    uint8_t config;          /**< Temperature resolution configuration register. */
    uint8_t reserved[3];     /**< Reserved bytes defined by the DS18B20 scratchpad format. */
    uint8_t crc;             /**< Dallas/Maxim CRC-8 of the first eight bytes. */
} ds18b20_scratchpad_t;

/**
 * @brief Start a DS18B20 temperature conversion.
 *
 * Issues the DS18B20 `Convert T` command to either a specific device or
 * all devices on the 1-Wire bus.
 *
 * When a ROM address is supplied, the device is selected using `Match ROM`.
 * When `rom` is NULL, the operation is broadcast using `Skip ROM`.
 *
 * The function returns after starting the conversion and does not wait for
 * the conversion to complete.
 *
 * @param[in] bus Pointer to the configured 1-Wire bus instance.
 * @param[in] rom Pointer to the target device's 8-byte ROM code.
 *            Pass NULL to start conversion on all devices.
 * @param[in] parasite_power `true` when the target device is parasite-powered;
 *            `false` when the device is externally powered.
 *
 * @return `ONEWIRE_OK` if the command was successfully issued.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @warning When `parasite_power` is true, the 1-Wire bus must provide a
 *          suitable strong pull-up immediately after the command.
 *
 * @note This function does not wait for the conversion to complete.
 *       Use `ds18b20_convert_wait()` when the application requires a
 *       completed conversion before continuing.
 */
onewire_status_t ds18b20_start_conversion(const onewire_bus_t *bus,
                                           const uint8_t rom[8],
                                           bool parasite_power);

/**
 * @brief Start a temperature conversion and wait for completion.
 *
 * Starts a DS18B20 temperature conversion and waits for the maximum
 * conversion time associated with the requested resolution.
 *
 * This function is useful when the application requires a fresh
 * temperature value immediately after the function returns.
 *
 * @param[in] bus Pointer to the configured 1-Wire bus instance.
 * @param[in] rom Pointer to the target device's 8-byte ROM code.
 *            Pass NULL to perform the conversion for all devices.
 * @param[in] parasite_power `true` when the target device is parasite-powered;
 *            `false` when the device is externally powered.
 * @param[in] resolution Temperature resolution used to determine the
 *            maximum conversion time.
 *
 * @return `ONEWIRE_OK` if the conversion was started successfully and
 *         the wait completed.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @warning A suitable strong pull-up is mandatory for parasite-powered
 *          operation during the conversion interval.
 */
onewire_status_t ds18b20_convert_wait(const onewire_bus_t *bus,
                                       const uint8_t rom[8],
                                       bool parasite_power,
                                       ds18b20_resolution_t resolution);

/**
 * @brief Read and validate the complete DS18B20 scratchpad.
 *
 * Reads all nine bytes of the DS18B20 scratchpad and validates the
 * embedded Dallas/Maxim CRC-8 before returning the data.
 *
 * @param[in]  bus Pointer to the configured 1-Wire bus instance.
 * @param[in]  rom Pointer to the target device's 8-byte ROM code.
 * @param[out] scratchpad Pointer to the structure receiving the scratchpad
 *             contents.
 *
 * @return `ONEWIRE_OK` if the scratchpad was read and the CRC was valid.
 * @return `ONEWIRE_ERR_CRC` if the scratchpad CRC validation failed.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @warning `scratchpad` must point to valid writable storage.
 */
onewire_status_t ds18b20_read_scratchpad(const onewire_bus_t *bus,
                                          const uint8_t rom[8],
                                          ds18b20_scratchpad_t *scratchpad);

/**
 * @brief Read the raw DS18B20 temperature value.
 *
 * Reads the DS18B20 scratchpad, validates its CRC, and extracts the signed
 * 16-bit temperature register.
 *
 * The returned value uses the DS18B20 native temperature representation,
 * where one least-significant bit corresponds to 1/16 °C at 12-bit
 * resolution.
 *
 * At lower resolutions, the least-significant temperature bits are not
 * significant according to the selected device resolution.
 *
 * @param[in]  bus Pointer to the configured 1-Wire bus instance.
 * @param[in]  rom Pointer to the target device's 8-byte ROM code.
 * @param[out] raw_temperature Pointer receiving the signed raw temperature.
 *
 * @return `ONEWIRE_OK` if the temperature was read successfully and the
 *         scratchpad CRC was valid.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @warning `raw_temperature` must point to valid writable storage.
 */
onewire_status_t ds18b20_read_raw(const onewire_bus_t *bus,
                                   const uint8_t rom[8],
                                   int16_t *raw_temperature);

/**
 * @brief Convert a raw DS18B20 temperature to milli-degrees Celsius.
 *
 * Converts the native signed DS18B20 temperature representation into
 * milli-degrees Celsius using integer arithmetic only.
 *
 * This function does not use floating-point operations and is therefore
 * suitable for small AVR targets where floating-point support would add
 * unnecessary code and runtime overhead.
 *
 * @param[in] raw_temperature Signed raw temperature value returned by
 *            `ds18b20_read_raw()`.
 *
 * @return Temperature in milli-degrees Celsius.
 *
 * @note The conversion assumes the standard DS18B20 temperature register
 *       format with a 1/16 °C least-significant bit.
 */
int32_t ds18b20_raw_to_milli_celsius(int16_t raw_temperature);

/**
 * @brief Write temperature alarm thresholds and resolution configuration.
 *
 * Writes the DS18B20 TH, TL, and configuration registers to the device's
 * scratchpad.
 *
 * The configuration byte is generated from the requested resolution.
 *
 * @param[in] bus Pointer to the configured 1-Wire bus instance.
 * @param[in] rom Pointer to the target device's 8-byte ROM code.
 * @param[in] th High-temperature alarm threshold in degrees Celsius.
 * @param[in] tl Low-temperature alarm threshold in degrees Celsius.
 * @param[in] resolution Desired temperature resolution.
 *
 * @return `ONEWIRE_OK` if the scratchpad was written successfully.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @note This function modifies the scratchpad only. Use
 *       `ds18b20_copy_scratchpad()` to persist the configuration in EEPROM.
 */
onewire_status_t ds18b20_write_scratchpad(const onewire_bus_t *bus,
                                           const uint8_t rom[8],
                                           int8_t th,
                                           int8_t tl,
                                           ds18b20_resolution_t resolution);

/**
 * @brief Copy DS18B20 scratchpad configuration to EEPROM.
 *
 * Copies the TH, TL, and configuration registers from the scratchpad
 * into the device's non-volatile EEPROM.
 *
 * @param[in] bus Pointer to the configured 1-Wire bus instance.
 * @param[in] rom Pointer to the target device's 8-byte ROM code.
 * @param[in] parasite_power `true` when the target device is parasite-powered;
 *            `false` when the device is externally powered.
 *
 * @return `ONEWIRE_OK` if the command was successfully issued.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @warning Parasite-powered devices require a suitable strong pull-up for
 *          the duration required by the DS18B20 to complete the EEPROM write.
 */
onewire_status_t ds18b20_copy_scratchpad(const onewire_bus_t *bus,
                                          const uint8_t rom[8],
                                          bool parasite_power);

/**
 * @brief Determine whether a DS18B20 is parasite-powered.
 *
 * Issues the DS18B20 `Read Power Supply` command and determines the device's
 * power mode.
 *
 * @param[in]  bus Pointer to the configured 1-Wire bus instance.
 * @param[in]  rom Pointer to the target device's 8-byte ROM code.
 * @param[out] parasite_powered Set to `true` when the device is parasite-powered,
 *              or `false` when powered from an external supply.
 *
 * @return `ONEWIRE_OK` if the power mode was read successfully.
 * @return A corresponding `onewire_status_t` error code otherwise.
 *
 * @note The DS18B20 reports a low level for parasite-powered operation and
 *       a high level for externally powered operation. This function converts
 *       that device-level result into the boolean output parameter.
 *
 * @warning `parasite_powered` must point to valid writable storage.
 */
onewire_status_t ds18b20_is_parasite_powered(const onewire_bus_t *bus,
                                              const uint8_t rom[8],
                                              bool *parasite_powered);

#ifdef __cplusplus
}
#endif

#endif /* AVR_DS18B20_H */