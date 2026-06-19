#ifndef VL53L4CD_TEMPLATE_H
#define VL53L4CD_TEMPLATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Units byte values expected by the existing I2C slave protocol. */
#define VL53L4CD_UNIT_INCH ((uint8_t)0x50u)
#define VL53L4CD_UNIT_CM   ((uint8_t)0x51u)
#define VL53L4CD_UNIT_MM   ((uint8_t)0x52u)

/* Protocol command IDs. */
#define VL53L4CD_CMD_GET_RANGING_RESULT     ((uint8_t)0x00u)
#define VL53L4CD_CMD_SET_RANGING_TIMING     ((uint8_t)0x01u)
#define VL53L4CD_CMD_START_OFFSET_CAL       ((uint8_t)0x02u)
#define VL53L4CD_CMD_START_XTALK_CAL        ((uint8_t)0x03u)
#define VL53L4CD_CMD_GET_EEPROM_DATA        ((uint8_t)0x04u)
#define VL53L4CD_CMD_RESTORE_FACTORY_CONFIG ((uint8_t)0x05u)
#define VL53L4CD_CMD_SET_THRESHOLDS         ((uint8_t)0x07u)
#define VL53L4CD_CMD_RESTART                ((uint8_t)0x08u)

#define VL53L4CD_RANGING_FRAME_SIZE ((uint8_t)20u)

typedef enum {
    VL53L4CD_OK = 0,
    VL53L4CD_ERR_ARG,
    VL53L4CD_ERR_IO,
    VL53L4CD_ERR_TIMEOUT
} vl53l4cd_status_t;

typedef uint8_t (*vl53l4cd_i2c_write_fn)(uint8_t address, const uint8_t *tx, uint8_t tx_len, void *user_ctx);
typedef uint8_t (*vl53l4cd_i2c_read_fn)(uint8_t address, uint8_t *rx, uint8_t rx_len, void *user_ctx);
typedef void (*vl53l4cd_delay_ms_fn)(uint16_t delay_ms, void *user_ctx);

typedef struct {
    vl53l4cd_i2c_write_fn i2c_write;
    vl53l4cd_i2c_read_fn i2c_read;
    vl53l4cd_delay_ms_fn delay_ms;
    void *user_ctx;
    uint16_t max_range_mm;
    uint16_t poll_interval_ms;
    uint16_t max_poll_count;
} vl53l4cd_template_cfg_t;

typedef struct {
    vl53l4cd_template_cfg_t cfg;
} vl53l4cd_template_t;

typedef struct {
    uint16_t range_mm;
    uint8_t status;
    uint16_t signal_kcps;
    uint16_t ambient_kcps;
    uint16_t sigma_mm;
} vl53l4cd_measurement_t;

/*
 * Measurement status post-processing done by vl53l4cd_template_get_measurement:
 * - status 3 (BELOW_THRESHOLD) => range_mm forced to 0
 * - status 2 (SIGNAL_LOW), 4 (PHASE_OUT_OF_LIMIT), 12 (SIGNAL_TOO_LOW)
 *   => range_mm forced to cfg.max_range_mm
 */

vl53l4cd_status_t vl53l4cd_template_init(vl53l4cd_template_t *ctx, const vl53l4cd_template_cfg_t *cfg);

vl53l4cd_status_t vl53l4cd_template_send_simple(vl53l4cd_template_t *ctx, uint8_t dev_address, uint8_t command_id);
vl53l4cd_status_t vl53l4cd_template_set_new_address(vl53l4cd_template_t *ctx, uint8_t dev_address, uint8_t new_address);
vl53l4cd_status_t vl53l4cd_template_set_ranging_timing(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t timebudget,
    uint16_t intermeasurement_time_ms
);
vl53l4cd_status_t vl53l4cd_template_start_offset_cal(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    int16_t cal_distance_mm,
    uint16_t samples_nbr
);
vl53l4cd_status_t vl53l4cd_template_start_xtalk_cal(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint16_t cal_distance_mm,
    uint16_t samples_nbr
);
vl53l4cd_status_t vl53l4cd_template_set_thresholds(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint16_t sigma,
    uint16_t signal_threshold
);

vl53l4cd_status_t vl53l4cd_template_read_ranging_frame(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t *frame,
    uint8_t frame_len
);

vl53l4cd_status_t vl53l4cd_template_get_measurement(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t units,
    vl53l4cd_measurement_t *measurement,
    uint16_t *attempts
);

vl53l4cd_status_t vl53l4cd_template_get_measurements(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    vl53l4cd_measurement_t *measurements,
    uint16_t *attempts,
    vl53l4cd_status_t *statuses
);

bool vl53l4cd_template_status_is_accepted(uint8_t status);
const char *vl53l4cd_template_status_text(uint8_t status);
uint16_t vl53l4cd_template_convert_range(uint16_t range_mm, uint8_t units);

void vl53l4cd_template_demo_run(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t units,
    uint16_t period_ms,
    uint16_t max_samples
);

void vl53l4cd_template_demo_run_all(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    uint16_t period_ms,
    uint16_t max_cycles
);

void vl53l4cd_template_demo_run_all_simultaneous(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    uint16_t wait_ms,
    uint16_t period_ms,
    uint16_t max_cycles
);

#ifdef __cplusplus
}
#endif

#endif /* VL53L4CD_TEMPLATE_H */