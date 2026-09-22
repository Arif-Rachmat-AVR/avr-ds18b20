/**
 * @file ds18b20.c
 * @brief DS18B20 digital temperature sensor driver implementation.
 *
 * @details Implements the public DS18B20 device API declared in `ds18b20.h`.
 *          1-Wire bus access is delegated to the `avr-onewire` library.
 *
 *          The implementation handles DS18B20-specific command sequences,
 *          scratchpad parsing, CRC validation, temperature conversion, and
 *          parasite-power-related operations.
 *
 * @author Arif Rachmat (ngaripar1203@gmail.com)
 * @date 2026-09-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2026 Muhammad Arif Rachmat
 *            Licensed under the MIT License (see LICENSE for details).
 *
 * @note This file should not be used independently of `ds18b20.h` and
 *       the `avr-onewire` library.
 */

#include "ds18b20.h"
#include "ds18b20.h"

#include <util/delay.h>

#define DS18B20_TCONV_9BIT_MS 94u
#define DS18B20_TCONV_10BIT_MS 188u
#define DS18B20_TCONV_11BIT_MS 375u
#define DS18B20_TCONV_12BIT_MS 750u

static onewire_status_t select_device(const onewire_bus_t *bus,
                                      const uint8_t rom[8]) {
    if (rom == NULL) {
        return onewire_skip_rom(bus);
    }

    /* The DS18B20 family code is fixed at 28h. */
    if (rom[0] != DS18B20_FAMILY_CODE) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    return onewire_match_rom(bus, rom);
}

static uint16_t conversion_time_ms(ds18b20_resolution_t resolution) {
    switch (resolution) {
    case DS18B20_RESOLUTION_9BIT:
        return DS18B20_TCONV_9BIT_MS;
    case DS18B20_RESOLUTION_10BIT:
        return DS18B20_TCONV_10BIT_MS;
    case DS18B20_RESOLUTION_11BIT:
        return DS18B20_TCONV_11BIT_MS;
    case DS18B20_RESOLUTION_12BIT:
        return DS18B20_TCONV_12BIT_MS;
    default:
        return 0u;
    }
}

static uint8_t resolution_to_config(ds18b20_resolution_t resolution) {
    switch (resolution) {
    case DS18B20_RESOLUTION_9BIT:
        return 0x1Fu;
    case DS18B20_RESOLUTION_10BIT:
        return 0x3Fu;
    case DS18B20_RESOLUTION_11BIT:
        return 0x5Fu;
    case DS18B20_RESOLUTION_12BIT:
        return 0x7Fu;
    default:
        return 0u;
    }
}

onewire_status_t ds18b20_start_conversion(const onewire_bus_t *bus,
                                          const uint8_t rom[8],
                                          bool parasite_power) {
    if (bus == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    onewire_status_t status = onewire_reset(bus);
    if (status != ONEWIRE_OK) {
        return status;
    }

    status = select_device(bus, rom);
    if (status != ONEWIRE_OK) {
        return status;
    }

    if (parasite_power) {
        /* Keep the final byte's timing-critical section active while the
         * strong-pull-up stage is asserted. */
        return onewire_write_byte_and_enable_strong_pullup(
            bus, DS18B20_CMD_CONVERT_T);
    }

    onewire_write_byte(bus, DS18B20_CMD_CONVERT_T);
    return ONEWIRE_OK;
}

onewire_status_t ds18b20_convert_wait(const onewire_bus_t *bus,
                                      const uint8_t rom[8], bool parasite_power,
                                      ds18b20_resolution_t resolution) {
    const uint16_t conversion_ms = conversion_time_ms(resolution);
    if (conversion_ms == 0u) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    onewire_status_t status =
        ds18b20_start_conversion(bus, rom, parasite_power);
    if (status != ONEWIRE_OK) {
        return status;
    }

    /* _delay_ms() has compile-time-constant arguments only; loop for
     * portability. */
    for (uint16_t i = 0u; i < conversion_ms; ++i) {
        _delay_ms(1.0);
    }

    if (parasite_power) {
        return onewire_set_strong_pullup(bus, false);
    }

    return ONEWIRE_OK;
}

onewire_status_t ds18b20_read_scratchpad(const onewire_bus_t *bus,
                                         const uint8_t rom[8],
                                         ds18b20_scratchpad_t *scratchpad) {
    if (bus == NULL || scratchpad == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    uint8_t raw[DS18B20_SCRATCHPAD_SIZE];
    onewire_status_t status = onewire_reset(bus);
    if (status != ONEWIRE_OK) {
        return status;
    }

    status = select_device(bus, rom);
    if (status != ONEWIRE_OK) {
        return status;
    }

    onewire_write_byte(bus, DS18B20_CMD_READ_SCRATCHPAD);
    onewire_read_block(bus, raw, sizeof raw);

    if (!onewire_crc8_valid(raw, sizeof raw)) {
        return ONEWIRE_ERR_CRC;
    }

    scratchpad->temperature_lsb = raw[0];
    scratchpad->temperature_msb = raw[1];
    scratchpad->th = (int8_t)raw[2];
    scratchpad->tl = (int8_t)raw[3];
    scratchpad->config = raw[4];
    scratchpad->reserved[0] = raw[5];
    scratchpad->reserved[1] = raw[6];
    scratchpad->reserved[2] = raw[7];
    scratchpad->crc = raw[8];

    return ONEWIRE_OK;
}

onewire_status_t ds18b20_read_raw(const onewire_bus_t *bus,
                                  const uint8_t rom[8],
                                  int16_t *raw_temperature) {
    if (raw_temperature == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    ds18b20_scratchpad_t scratchpad;
    onewire_status_t status = ds18b20_read_scratchpad(bus, rom, &scratchpad);
    if (status != ONEWIRE_OK) {
        return status;
    }

    const uint16_t raw = (uint16_t)scratchpad.temperature_lsb |
                         ((uint16_t)scratchpad.temperature_msb << 8u);
    *raw_temperature = (int16_t)raw;
    return ONEWIRE_OK;
}

int32_t ds18b20_raw_to_milli_celsius(int16_t raw_temperature) {
    return (int32_t)(((int32_t)raw_temperature * 625L) / 10L);
}

onewire_status_t ds18b20_write_scratchpad(const onewire_bus_t *bus,
                                          const uint8_t rom[8], int8_t th,
                                          int8_t tl,
                                          ds18b20_resolution_t resolution) {
    const uint8_t config = resolution_to_config(resolution);
    if (config == 0u || bus == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    onewire_status_t status = onewire_reset(bus);
    if (status != ONEWIRE_OK) {
        return status;
    }

    status = select_device(bus, rom);
    if (status != ONEWIRE_OK) {
        return status;
    }

    onewire_write_byte(bus, DS18B20_CMD_WRITE_SCRATCHPAD);
    onewire_write_byte(bus, (uint8_t)th);
    onewire_write_byte(bus, (uint8_t)tl);
    onewire_write_byte(bus, config);

    return ONEWIRE_OK;
}

onewire_status_t ds18b20_copy_scratchpad(const onewire_bus_t *bus,
                                         const uint8_t rom[8],
                                         bool parasite_power) {
    if (bus == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    onewire_status_t status = onewire_reset(bus);
    if (status != ONEWIRE_OK) {
        return status;
    }

    status = select_device(bus, rom);
    if (status != ONEWIRE_OK) {
        return status;
    }

    if (parasite_power) {
        /* As with Convert T, assert strong pull-up as part of the final
         * command-byte timing operation. */
        status = onewire_write_byte_and_enable_strong_pullup(
            bus, DS18B20_CMD_COPY_SCRATCHPAD);
        if (status != ONEWIRE_OK) {
            return status;
        }
    } else {
        onewire_write_byte(bus, DS18B20_CMD_COPY_SCRATCHPAD);
    }

    /* Copy Scratchpad has a maximum write time of 10 ms.  No new bus
     * transaction is allowed until that operation has completed. */
    for (uint8_t i = 0u; i < 10u; ++i) {
        _delay_ms(1.0);
    }

    if (parasite_power) {
        status = onewire_set_strong_pullup(bus, false);
        return status;
    }

    return ONEWIRE_OK;
}

onewire_status_t ds18b20_is_parasite_powered(const onewire_bus_t *bus,
                                             const uint8_t rom[8],
                                             bool *parasite_powered) {
    if (bus == NULL || parasite_powered == NULL) {
        return ONEWIRE_ERR_INVALID_ARG;
    }

    onewire_status_t status = onewire_reset(bus);
    if (status != ONEWIRE_OK) {
        return status;
    }

    status = select_device(bus, rom);
    if (status != ONEWIRE_OK) {
        return status;
    }

    onewire_write_byte(bus, DS18B20_CMD_READ_POWER_SUPPLY);
    *parasite_powered = (onewire_read_bit(bus) == 0u);
    return ONEWIRE_OK;
}
