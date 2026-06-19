# VL53L4CD Board and API Library

This repository contains:

- a VL53L4CD breakout board design,
- board images and schematics,
- and a reusable C template module for VL53L4CD integration over I2C.

## Included Files

Main reusable module files:

- `vl53l4cd_template.h`
- `vl53l4cd_template.c`
- `vl53l4cd_template_demo.c`

## Board Overview

The breakout board exposes power and I2C through connector J1 and includes onboard weak 10K I2C pull-ups.
If you want to use external pull-ups, cut jumpers `J2` and `J3`.

### J1 Connector Pins

- Pin 1 (square): VDD 3V to 5.5V
- Pin 2: SDA
- Pin 3: SCL
- Pin 4: NU
- Pin 5: GND

## Images

![Breakout board back](images/breakout%20brd%20back.jpg)
![Breakout board front](images/breakout%20brd%20front.jpg)
![Multiple sensors on the same I2C bus](images/multiple%20sensors%20on%20the%20same%20I2C%20bus.jpeg)

## Quick Start

1. Copy `vl53l4cd_template.h` and `vl53l4cd_template.c` into your project.
2. Implement the I2C read/write callbacks.
3. Optionally provide a delay callback.
4. Initialize `vl53l4cd_template_t`.
5. Use the template API to read measurements.

## API Highlights

- `vl53l4cd_template_init`
- `vl53l4cd_template_get_measurement`
- `vl53l4cd_template_get_measurements`
- `vl53l4cd_template_set_new_address`
- `vl53l4cd_template_set_ranging_timing`
- `vl53l4cd_template_set_thresholds`

## Documentation

For the full documentation, detailed protocol description, status behavior, demo usage, and platform integration examples, see:

- [`VL53L4CD_README.md`](VL53L4CD_README.md)
