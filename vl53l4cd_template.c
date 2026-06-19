#include "vl53l4cd_template.h"

#include <string.h>

static vl53l4cd_status_t write_cmd(vl53l4cd_template_t *ctx, uint8_t dev_address, const uint8_t *tx, uint8_t tx_len)
{
    if ((ctx == NULL) || (tx == NULL) || (tx_len == 0u)) {
        return VL53L4CD_ERR_ARG;
    }

    if (ctx->cfg.i2c_write(dev_address, tx, tx_len, ctx->cfg.user_ctx) != 0u) {
        return VL53L4CD_ERR_IO;
    }

    return VL53L4CD_OK;
}

static vl53l4cd_status_t read_frame(vl53l4cd_template_t *ctx, uint8_t dev_address, uint8_t *rx, uint8_t rx_len)
{
    if ((ctx == NULL) || (rx == NULL) || (rx_len == 0u)) {
        return VL53L4CD_ERR_ARG;
    }

    if (ctx->cfg.i2c_read(dev_address, rx, rx_len, ctx->cfg.user_ctx) != 0u) {
        return VL53L4CD_ERR_IO;
    }

    return VL53L4CD_OK;
}

static void set_u16_be(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)((value >> 8) & 0xFFu);
    dst[1] = (uint8_t)(value & 0xFFu);
}

static uint16_t get_u16_be(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8) | src[1]);
}

vl53l4cd_status_t vl53l4cd_template_init(vl53l4cd_template_t *ctx, const vl53l4cd_template_cfg_t *cfg)
{
    if ((ctx == NULL) || (cfg == NULL) || (cfg->i2c_write == NULL) || (cfg->i2c_read == NULL)) {
        return VL53L4CD_ERR_ARG;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = *cfg;

    if (ctx->cfg.max_range_mm == 0u) {
        ctx->cfg.max_range_mm = 1300u;
    }
    if (ctx->cfg.poll_interval_ms == 0u) {
        ctx->cfg.poll_interval_ms = 10u;
    }
    if (ctx->cfg.max_poll_count == 0u) {
        ctx->cfg.max_poll_count = 100u;
    }

    return VL53L4CD_OK;
}

vl53l4cd_status_t vl53l4cd_template_send_simple(vl53l4cd_template_t *ctx, uint8_t dev_address, uint8_t command_id)
{
    uint8_t tx[1];
    tx[0] = command_id;
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_set_new_address(vl53l4cd_template_t *ctx, uint8_t dev_address, uint8_t new_address)
{
    uint8_t tx[2];
    vl53l4cd_status_t st;

    tx[0] = VL53L4CD_CMD_GET_RANGING_RESULT;

    tx[1] = 0xA0u;
    st = write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
    if (st != VL53L4CD_OK) return st;

    tx[1] = 0xAAu;
    st = write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
    if (st != VL53L4CD_OK) return st;

    tx[1] = 0xA5u;
    st = write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
    if (st != VL53L4CD_OK) return st;

    tx[1] = new_address;
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_set_ranging_timing(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t timebudget,
    uint16_t intermeasurement_time_ms
)
{
    uint8_t tx[4];
    tx[0] = VL53L4CD_CMD_SET_RANGING_TIMING;
    tx[1] = timebudget;
    set_u16_be(&tx[2], intermeasurement_time_ms);
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_start_offset_cal(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    int16_t cal_distance_mm,
    uint16_t samples_nbr
)
{
    uint8_t tx[5];
    tx[0] = VL53L4CD_CMD_START_OFFSET_CAL;
    set_u16_be(&tx[1], (uint16_t)cal_distance_mm);
    set_u16_be(&tx[3], samples_nbr);
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_start_xtalk_cal(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint16_t cal_distance_mm,
    uint16_t samples_nbr
)
{
    uint8_t tx[5];
    tx[0] = VL53L4CD_CMD_START_XTALK_CAL;
    set_u16_be(&tx[1], cal_distance_mm);
    set_u16_be(&tx[3], samples_nbr);
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_set_thresholds(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint16_t sigma,
    uint16_t signal_threshold
)
{
    uint8_t tx[5];
    tx[0] = VL53L4CD_CMD_SET_THRESHOLDS;
    set_u16_be(&tx[1], sigma);
    set_u16_be(&tx[3], signal_threshold);
    return write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
}

vl53l4cd_status_t vl53l4cd_template_read_ranging_frame(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t *frame,
    uint8_t frame_len
)
{
    if ((frame == NULL) || (frame_len < 9u)) {
        return VL53L4CD_ERR_ARG;
    }

    return read_frame(ctx, dev_address, frame, frame_len);
}

vl53l4cd_status_t vl53l4cd_template_get_measurement(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t units,
    vl53l4cd_measurement_t *measurement,
    uint16_t *attempts
)
{
    uint8_t tx[2];
    uint8_t frame[VL53L4CD_RANGING_FRAME_SIZE];
    uint16_t i;
    vl53l4cd_status_t st;

    if ((ctx == NULL) || (measurement == NULL)) {
        return VL53L4CD_ERR_ARG;
    }

    tx[0] = VL53L4CD_CMD_GET_RANGING_RESULT;
    tx[1] = units;

    st = write_cmd(ctx, dev_address, tx, (uint8_t)sizeof(tx));
    if (st != VL53L4CD_OK) {
        return st;
    }

    for (i = 0u; i < ctx->cfg.max_poll_count; i++) {
        if (ctx->cfg.delay_ms != NULL) {
            ctx->cfg.delay_ms(ctx->cfg.poll_interval_ms, ctx->cfg.user_ctx);
        }

        st = read_frame(ctx, dev_address, frame, (uint8_t)sizeof(frame));
        if (st != VL53L4CD_OK) {
            return st;
        }

        measurement->range_mm = get_u16_be(&frame[0]);
        measurement->status = frame[2];
        measurement->signal_kcps = get_u16_be(&frame[3]);
        measurement->ambient_kcps = get_u16_be(&frame[5]);
        measurement->sigma_mm = get_u16_be(&frame[7]);

        if (vl53l4cd_template_status_is_accepted(measurement->status)) {
            if (measurement->status == 3u) {
                measurement->range_mm = 0u;
            } else if ((measurement->status == 2u) || (measurement->status == 4u) || (measurement->status == 12u)) {
                measurement->range_mm = ctx->cfg.max_range_mm;
            }

            if (attempts != NULL) {
                *attempts = (uint16_t)(i + 1u);
            }
            return VL53L4CD_OK;
        }
    }

    if (attempts != NULL) {
        *attempts = ctx->cfg.max_poll_count;
    }
    return VL53L4CD_ERR_TIMEOUT;
}

vl53l4cd_status_t vl53l4cd_template_get_measurements(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    vl53l4cd_measurement_t *measurements,
    uint16_t *attempts,
    vl53l4cd_status_t *statuses
)
{
    uint8_t index;
    vl53l4cd_status_t overall_status = VL53L4CD_OK;

    if ((ctx == NULL) || (dev_addresses == NULL) || (measurements == NULL) || (dev_count == 0u)) {
        return VL53L4CD_ERR_ARG;
    }

    for (index = 0u; index < dev_count; index++) {
        uint16_t *attempt_slot = (attempts != NULL) ? &attempts[index] : NULL;
        vl53l4cd_status_t st = vl53l4cd_template_get_measurement(
            ctx,
            dev_addresses[index],
            units,
            &measurements[index],
            attempt_slot
        );

        if (statuses != NULL) {
            statuses[index] = st;
        }

        if ((st != VL53L4CD_OK) && (overall_status == VL53L4CD_OK)) {
            overall_status = st;
        }
    }

    return overall_status;
}

bool vl53l4cd_template_status_is_accepted(uint8_t status)
{
    return (status == 0u) || (status == 1u) || (status == 2u) || (status == 3u) || (status == 4u) || (status == 12u);
}

const char *vl53l4cd_template_status_text(uint8_t status)
{
    switch (status) {
        case 0u:   return "VALID";
        case 1u:   return "SIGMA_HIGH";
        case 2u:   return "SIGNAL_LOW";
        case 3u:   return "BELOW_THRESHOLD";
        case 4u:   return "PHASE_OUT_OF_LIMIT";
        case 5u:   return "HARDWARE_FAIL";
        case 6u:   return "PHASE_VALID_NOWRAP";
        case 7u:   return "WRAPPED_TARGET";
        case 8u:   return "PROCESSING_FAIL";
        case 9u:   return "CROSSTALK_FAIL";
        case 10u:  return "INTERRUPT_ERROR";
        case 11u:  return "MERGED_TARGET";
        case 12u:  return "SIGNAL_TOO_LOW";
        case 255u: return "OTHER_ERROR";
        default:   return "UNKNOWN";
    }
}

uint16_t vl53l4cd_template_convert_range(uint16_t range_mm, uint8_t units)
{
    switch (units) {
        case VL53L4CD_UNIT_CM:
            return (uint16_t)((range_mm + 5u) / 10u);
        case VL53L4CD_UNIT_INCH:
            return (uint16_t)((((uint32_t)range_mm * 100u) + 1270u) / 2540u);
        case VL53L4CD_UNIT_MM:
        default:
            return range_mm;
    }
}