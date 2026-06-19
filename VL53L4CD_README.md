# VL53L4CD Board and API library Description

- The VL53L4CD breakout board exposes power and I2C through connector J1 and includes onboard weak I2C pull-ups that can be disconnected when using external pull-ups.

## Board Images

![Breakout board back](images/breakout%20brd%20back.jpg)
![Breakout board front](images/breakout%20brd%20front.jpg)
![Multiple sensors on the same I2C bus](images/multiple%20sensors%20on%20the%20same%20I2C%20bus.jpeg)

## Board Schematics

- breakout schematics.pdf

## J1 Connector Pins

- Pin 1 (square): VDD 3V to 5.5V
- Pin 2: SDA
- Pin 3: SCL
- Pin 4: NU
- Pin 5: GND

## I2C Pull-ups

R1 and R2 are weak (10K) pull-ups. If you plan to use external pull-ups, cut jumpers J2 and J3.

## VL53L4CD Reusable Template Module

This folder now includes a reusable C module that can be copied into other projects using the same VL53L4CD I2C template API protocol.

Files:

- `vl53l4cd_template.h`
- `vl53l4cd_template.c`
- `vl53l4cd_template_demo.c`

The template is transport-agnostic: it does not include platform-specific headers. You inject platform behavior using callbacks for I2C read/write and optional delay.

## What This Module Implements

The template API wraps the same protocol already used in this project:

- `0x00`: get ranging result (`units` byte payload)
- `0x00` special sequence: set new address (`A0`, `AA`, `A5`, `new_addr`)
- `0x01`: set ranging timing (`timebudget`, `intermeasurement_time`)
- `0x02`: start offset calibration
- `0x03`: start xtalk calibration
- `0x04`: get EEPROM data
- `0x05`: restore factory config
- `0x07`: set sigma/signal thresholds
- `0x08`: restart

It also parses ranging frames in the same layout used by your existing `main.c`:

- bytes `[0..1]`: range (mm, big-endian)
- byte `[2]`: status
- bytes `[3..4]`: signal (kcps)
- bytes `[5..6]`: ambient (kcps)
- bytes `[7..8]`: sigma (mm)

## Quick Start

1. Copy `vl53l4cd_template.h` and `vl53l4cd_template.c` into the new project.
2. Add the files to the project build.
3. Implement the callback wrappers for your I2C driver.
4. Initialize `vl53l4cd_template_t` once.
5. Use the high-level template API functions for actions and ranging.

## Example: PSoC Creator Integration

This snippet shows a typical integration with PSoC Creator I2C APIs.

```c
#include "vl53l4cd_template.h"
#include "Generated_Source/PSoC5/I2C.h"
#include <project.h>

static uint8_t psoc_i2c_write(uint8_t address, const uint8_t *tx, uint8_t tx_len, void *user_ctx)
{
    (void)user_ctx;
    uint8_t st = I2C_MasterWriteBuf(address, (uint8_t *)tx, tx_len, I2C_MODE_COMPLETE_XFER);
    if (st != I2C_MSTR_NO_ERROR) {
        return st;
    }

    do {
        st = I2C_MasterStatus();
    } while ((st & (I2C_MSTAT_WR_CMPLT | I2C_MSTAT_ERR_XFER)) == 0u);

    if (st & I2C_MSTAT_ERR_MASK) {
        I2C_MasterClearStatus();
        return 1u;
    }

    I2C_MasterClearStatus();
    return 0u;
}

static uint8_t psoc_i2c_read(uint8_t address, uint8_t *rx, uint8_t rx_len, void *user_ctx)
{
    (void)user_ctx;
    uint8_t st = I2C_MasterReadBuf(address, rx, rx_len, I2C_MODE_COMPLETE_XFER);
    if (st != I2C_MSTR_NO_ERROR) {
        return st;
    }

    do {
        st = I2C_MasterStatus();
    } while ((st & (I2C_MSTAT_RD_CMPLT | I2C_MSTAT_ERR_XFER)) == 0u);

    if (st & I2C_MSTAT_ERR_MASK) {
        I2C_MasterClearStatus();
        return 1u;
    }

    I2C_MasterClearStatus();
    return 0u;
}

static void psoc_delay_ms(uint16_t delay_ms, void *user_ctx)
{
    (void)user_ctx;
    CyDelay((uint32)delay_ms);
}

void app_init(void)
{
    vl53l4cd_template_t sensor;
    vl53l4cd_template_cfg_t cfg;
    vl53l4cd_measurement_t m;
    uint16_t attempts;

    cfg.i2c_write = psoc_i2c_write;
    cfg.i2c_read = psoc_i2c_read;
    cfg.delay_ms = psoc_delay_ms;
    cfg.user_ctx = NULL;
    cfg.max_range_mm = 1300u;
    cfg.poll_interval_ms = 10u;
    cfg.max_poll_count = 100u;

    if (vl53l4cd_template_init(&sensor, &cfg) != VL53L4CD_OK) {
        return;
    }

    if (vl53l4cd_template_get_measurement(&sensor, 0x70u, VL53L4CD_UNIT_MM, &m, &attempts) == VL53L4CD_OK) {
        uint16_t value_mm = m.range_mm;
        (void)value_mm;
    }
}
```

## Example: Arduino Integration

This snippet shows using the template with Arduino `Wire` and `delay`.

```c
#include <Arduino.h>
#include <Wire.h>
#include "vl53l4cd_template.h"

static uint8_t arduino_i2c_write(uint8_t address, const uint8_t *tx, uint8_t tx_len, void *user_ctx)
{
    TwoWire *wire = (TwoWire *)user_ctx;
    wire->beginTransmission(address);
    size_t written = wire->write(tx, tx_len);
    uint8_t end_status = (uint8_t)wire->endTransmission();

    return (written == tx_len && end_status == 0u) ? 0u : 1u;
}

static uint8_t arduino_i2c_read(uint8_t address, uint8_t *rx, uint8_t rx_len, void *user_ctx)
{
    TwoWire *wire = (TwoWire *)user_ctx;
    uint8_t requested = (uint8_t)wire->requestFrom((int)address, (int)rx_len);
    if (requested != rx_len) {
        return 1u;
    }

    for (uint8_t i = 0u; i < rx_len; ++i) {
        if (!wire->available()) {
            return 1u;
        }
        rx[i] = (uint8_t)wire->read();
    }

    return 0u;
}

static void arduino_delay_ms(uint16_t delay_ms, void *user_ctx)
{
    (void)user_ctx;
    delay((unsigned long)delay_ms);
}

void sensor_setup_example(void)
{
    static vl53l4cd_template_t sensor;
    vl53l4cd_template_cfg_t cfg;

    Wire.begin();

    cfg.i2c_write = arduino_i2c_write;
    cfg.i2c_read = arduino_i2c_read;
    cfg.delay_ms = arduino_delay_ms;
    cfg.user_ctx = &Wire;
    cfg.max_range_mm = 1300u;
    cfg.poll_interval_ms = 10u;
    cfg.max_poll_count = 100u;

    (void)vl53l4cd_template_init(&sensor, &cfg);
}
```

## Example: Generic C Platform Integration

This snippet shows a generic wrapper style where your board support package (BSP) provides low-level I2C and delay functions.

```c
#include "vl53l4cd_template.h"
#include "bsp_i2c.h"
#include "bsp_time.h"

typedef struct {
    int bus_id;
} app_i2c_ctx_t;

static uint8_t generic_i2c_write(uint8_t address, const uint8_t *tx, uint8_t tx_len, void *user_ctx)
{
    const app_i2c_ctx_t *ctx = (const app_i2c_ctx_t *)user_ctx;
    int rc = bsp_i2c_write(ctx->bus_id, address, tx, tx_len);
    return (rc == 0) ? 0u : 1u;
}

static uint8_t generic_i2c_read(uint8_t address, uint8_t *rx, uint8_t rx_len, void *user_ctx)
{
    const app_i2c_ctx_t *ctx = (const app_i2c_ctx_t *)user_ctx;
    int rc = bsp_i2c_read(ctx->bus_id, address, rx, rx_len);
    return (rc == 0) ? 0u : 1u;
}

static void generic_delay_ms(uint16_t delay_ms, void *user_ctx)
{
    (void)user_ctx;
    bsp_delay_ms((uint32_t)delay_ms);
}

void app_sensor_init(void)
{
    static app_i2c_ctx_t i2c_ctx = { .bus_id = 1 };
    static vl53l4cd_template_t sensor;
    vl53l4cd_template_cfg_t cfg = {
        .i2c_write = generic_i2c_write,
        .i2c_read = generic_i2c_read,
        .delay_ms = generic_delay_ms,
        .user_ctx = &i2c_ctx,
        .max_range_mm = 1300u,
        .poll_interval_ms = 10u,
        .max_poll_count = 100u
    };

    (void)vl53l4cd_template_init(&sensor, &cfg);
}
```

## API Overview

Initialization:

- `vl53l4cd_template_init`

Core template API helpers:

- `vl53l4cd_template_send_simple`
- `vl53l4cd_template_set_new_address`
- `vl53l4cd_template_set_ranging_timing`
- `vl53l4cd_template_start_offset_cal`
- `vl53l4cd_template_start_xtalk_cal`
- `vl53l4cd_template_set_thresholds`

Ranging:

- `vl53l4cd_template_read_ranging_frame`
- `vl53l4cd_template_get_measurement`
- `vl53l4cd_template_get_measurements`

Utilities:

- `vl53l4cd_template_status_is_accepted`
- `vl53l4cd_template_status_text`
- `vl53l4cd_template_convert_range`

## Demo Function (One-Shot + Continuous)

The file `vl53l4cd_template_demo.c` includes two minimal entry points:

- `vl53l4cd_template_demo_run(...)`
- `vl53l4cd_template_demo_run_all(...)`
- `vl53l4cd_template_demo_run_all_simultaneous(...)`

Behavior:

- Performs one one-shot measurement first
- Then runs a continuous loop for `max_samples` iterations
- Uses `period_ms` delay between samples
- Prints parsed values using the same template API

Example call:

```c
vl53l4cd_template_demo_run(&sensor, 0x70u, VL53L4CD_UNIT_MM, 200u, 20u);
```

Short simultaneous usage snippet:

```c
static const uint8_t sensor_addresses[] = { 0x70u, 0x71u, 0x72u };

/* One cycle: send start to all devices, wait once, then read all results. */
vl53l4cd_template_demo_run_all_simultaneous(
    &sensor,
    sensor_addresses,
    (uint8_t)(sizeof(sensor_addresses) / sizeof(sensor_addresses[0])),
    VL53L4CD_UNIT_MM,
    35u,  /* wait_ms: longest timebudget + guard */
    200u, /* period_ms between cycles */
    10u   /* max_cycles */
);
```

## Example: Ranging All Devices

If you already know the sensor addresses, you can call the transport-agnostic bulk helper directly and then optionally use the demo helper for quick logging.

Direct template API example:

```c
static const uint8_t sensor_addresses[] = { 0x70u, 0x71u, 0x72u };
vl53l4cd_measurement_t measurements[3];
uint16_t attempts[3];
vl53l4cd_status_t statuses[3];

if (vl53l4cd_template_get_measurements(
        &sensor,
        sensor_addresses,
        (uint8_t)(sizeof(sensor_addresses) / sizeof(sensor_addresses[0])),
        VL53L4CD_UNIT_MM,
        measurements,
        attempts,
        statuses
    ) != VL53L4CD_OK) {
    /* At least one device failed. Inspect statuses[i]. */
}
```

Demo helper example:

```c
static const uint8_t sensor_addresses[] = { 0x70u, 0x71u, 0x72u };

vl53l4cd_template_demo_run_all(
    &sensor,
    sensor_addresses,
    (uint8_t)(sizeof(sensor_addresses) / sizeof(sensor_addresses[0])),
    VL53L4CD_UNIT_MM,
    200u,
    10u
);
```

Behavior:

- Runs `10` acquisition cycles
- Reads devices `0x70`, `0x71`, and `0x72` in each cycle
- Waits `200 ms` between cycles
- Prints range, status text, signal, ambient, sigma, and poll attempts for each device
- The reusable bulk helper fills one measurement, attempt counter, and status per address

This makes it easy to validate integration quickly in a new project before wiring your full application logic.

## Status Handling Behavior

`vl53l4cd_template_get_measurement` follows your current logic:

- Accepted statuses: `0`, `1`, `2`, `3`, `4`, `12`
- For status `3` (`BELOW_THRESHOLD`), returned range is forced to `0`
- For status `2` (`SIGNAL_LOW`), `4` (`PHASE_OUT_OF_LIMIT`), and `12` (`SIGNAL_TOO_LOW`), returned range is forced to `max_range_mm`
- Any other status keeps polling until `max_poll_count` is reached

## Porting Checklist for a New Project

- Add both template files to the project.
- Ensure callbacks return `0` on success.
- Confirm device I2C address (default in this project was `0x70`).
- Confirm the template API command-ID mapping of your target slave firmware.
- Tune `poll_interval_ms`, `max_poll_count`, and `max_range_mm` to your system.
- Route the parsed result to your application logging or telemetry.

## Notes

- This module is plain C and does not depend on dynamic memory.
- It is safe to keep one context per sensor/device instance.
- If your new target uses a different protocol framing, adapt only this module and keep application code unchanged.

