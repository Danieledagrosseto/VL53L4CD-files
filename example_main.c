#include "project.h"
#include "Generated_Source/PSoC5/I2C.h"
#include "vl53l4cd_template.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define EXAMPLE_MAX_SENSORS ((uint8_t)16u)

static uint8_t psoc_i2c_write(uint8_t address, const uint8_t *tx, uint8_t tx_len, void *user_ctx)
{
    uint8_t st;
    (void)user_ctx;

    st = I2C_MasterWriteBuf(address, (uint8_t *)tx, tx_len, I2C_MODE_COMPLETE_XFER);
    if (st != I2C_MSTR_NO_ERROR) {
        return 1u;
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
    uint8_t st;
    (void)user_ctx;

    st = I2C_MasterReadBuf(address, rx, rx_len, I2C_MODE_COMPLETE_XFER);
    if (st != I2C_MSTR_NO_ERROR) {
        return 1u;
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

static bool example_address_is_bno(uint8_t address)
{
    return (address == 0x28u) || (address == 0x29u);
}

static bool example_verify_vl53_device(vl53l4cd_template_t *ctx, uint8_t address)
{
    uint8_t frame[VL53L4CD_RANGING_FRAME_SIZE];

    if (ctx == NULL) {
        return false;
    }

    if (vl53l4cd_template_send_simple(ctx, address, VL53L4CD_CMD_GET_EEPROM_DATA) != VL53L4CD_OK) {
        return false;
    }

    if (ctx->cfg.delay_ms != NULL) {
        ctx->cfg.delay_ms(20u, ctx->cfg.user_ctx);
    }

    if (vl53l4cd_template_read_ranging_frame(ctx, address, frame, (uint8_t)sizeof(frame)) != VL53L4CD_OK) {
        return false;
    }

    if (frame[0] != address) {
        return false;
    }

    return true;
}

static uint8_t example_scan_i2c_devices(uint8_t *addresses, uint8_t max_count)
{
    uint8_t address;
    uint8_t count = 0u;

    if ((addresses == NULL) || (max_count == 0u)) {
        return 0u;
    }

    for (address = 0x08u; address <= 0x7Fu; address++) {
        uint8_t st;

        if (example_address_is_bno(address)) {
            continue;
        }

        st = I2C_MasterSendStart(address, I2C_WRITE_XFER_MODE);
        if (st == I2C_MSTR_NO_ERROR) {
            I2C_MasterSendStop();
            if (count < max_count) {
                addresses[count++] = address;
            }
        } else {
            I2C_MasterSendStop();
        }

        CyDelay(2u);
    }

    return count;
}

static uint8_t example_filter_vl53_devices(vl53l4cd_template_t *ctx, uint8_t *addresses, uint8_t count)
{
    uint8_t read_index;
    uint8_t write_index = 0u;

    if ((ctx == NULL) || (addresses == NULL)) {
        return 0u;
    }

    for (read_index = 0u; read_index < count; read_index++) {
        if (example_verify_vl53_device(ctx, addresses[read_index])) {
            addresses[write_index++] = addresses[read_index];
        }
    }

    return write_index;
}

int main(void)
{
    vl53l4cd_template_t sensor;
    vl53l4cd_template_cfg_t cfg;
    vl53l4cd_measurement_t measurement;
    vl53l4cd_measurement_t measurements[EXAMPLE_MAX_SENSORS];
    vl53l4cd_status_t statuses[EXAMPLE_MAX_SENSORS];
    uint16_t attempts = 0u;
    uint16_t all_attempts[EXAMPLE_MAX_SENSORS];
    uint8_t sensor_addresses[EXAMPLE_MAX_SENSORS];
    uint8_t sensor_count;
    uint8_t index;

    CyGlobalIntEnable;
    UART_Start();
    I2C_Start();

    cfg.i2c_write = psoc_i2c_write;
    cfg.i2c_read = psoc_i2c_read;
    cfg.delay_ms = psoc_delay_ms;
    cfg.user_ctx = NULL;
    cfg.max_range_mm = 1300u;
    cfg.poll_interval_ms = 10u;
    cfg.max_poll_count = 100u;

    if (vl53l4cd_template_init(&sensor, &cfg) != VL53L4CD_OK) {
        printf("Template init failed\r\n");
        for(;;) {
        }
    }

    sensor_count = example_scan_i2c_devices(sensor_addresses, EXAMPLE_MAX_SENSORS);
    printf("Detected %u generic I2C device(s) for VL53 example\r\n", sensor_count);

    for (index = 0u; index < sensor_count; index++) {
        printf("  detected: 0x%02X\r\n", sensor_addresses[index]);
    }

    sensor_count = example_filter_vl53_devices(&sensor, sensor_addresses, sensor_count);
    printf("Verified %u VL53-compatible device(s)\r\n", sensor_count);

    for (index = 0u; index < sensor_count; index++) {
        printf("  VL53: 0x%02X\r\n", sensor_addresses[index]);
    }

    if (sensor_count == 0u) {
        printf("No VL53-compatible devices detected on I2C bus\r\n");
        for(;;) {
            CyDelay(1000u);
        }
    }

    if (vl53l4cd_template_get_measurement(&sensor, sensor_addresses[0], VL53L4CD_UNIT_MM, &measurement, &attempts) == VL53L4CD_OK) {
        printf("Single: range=%u mm status=%u (%s) attempts=%u\r\n",
               measurement.range_mm,
               measurement.status,
               vl53l4cd_template_status_text(measurement.status),
               attempts);
    } else {
        printf("Single[0x%02X]: failed\r\n", sensor_addresses[0]);
    }

    (void)vl53l4cd_template_get_measurements(
        &sensor,
        sensor_addresses,
        sensor_count,
        VL53L4CD_UNIT_MM,
        measurements,
        all_attempts,
        statuses
    );

    for (index = 0u; index < sensor_count; index++) {
        if (statuses[index] == VL53L4CD_OK) {
            printf("All[0x%02X]: range=%u mm status=%u (%s) attempts=%u\r\n",
                   sensor_addresses[index],
                   measurements[index].range_mm,
                   measurements[index].status,
                   vl53l4cd_template_status_text(measurements[index].status),
                   all_attempts[index]);
        } else {
            printf("All[0x%02X]: failed (%d)\r\n",
                   sensor_addresses[index],
                   (int)statuses[index]);
        }
    }

    for(;;) {
        CyDelay(1000u);
    }
}