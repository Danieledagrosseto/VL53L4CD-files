#include "vl53l4cd_template.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool demo_read_valid_simultaneous_frame(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t *frame,
    uint8_t frame_len
)
{
    uint16_t attempt;

    if ((ctx == NULL) || (frame == NULL) || (frame_len < 9u)) {
        return false;
    }

    for (attempt = 0u; attempt < ctx->cfg.max_poll_count; attempt++) {
        if (vl53l4cd_template_read_ranging_frame(ctx, dev_address, frame, frame_len) != VL53L4CD_OK) {
            return false;
        }

        if (vl53l4cd_template_status_is_accepted(frame[2])) {
            return true;
        }

        if (ctx->cfg.delay_ms != NULL) {
            ctx->cfg.delay_ms(ctx->cfg.poll_interval_ms, ctx->cfg.user_ctx);
        }
    }

    return false;
}

static void demo_print_ranging_frame(
    uint8_t dev_address,
    uint8_t units,
    const uint8_t *frame,
    uint8_t frame_len,
    uint16_t max_range_mm
)
{
    uint16_t range_mm;
    uint8_t status;
    uint16_t signal_kcps;
    uint16_t ambient_kcps;
    uint16_t sigma_mm;

    if ((frame == NULL) || (frame_len < 9u)) {
        printf("  [0x%02X] invalid frame\r\n", dev_address);
        return;
    }

    range_mm = (uint16_t)(((uint16_t)frame[0] << 8) | frame[1]);
    status = frame[2];
    signal_kcps = (uint16_t)(((uint16_t)frame[3] << 8) | frame[4]);
    ambient_kcps = (uint16_t)(((uint16_t)frame[5] << 8) | frame[6]);
    sigma_mm = (uint16_t)(((uint16_t)frame[7] << 8) | frame[8]);

    if (vl53l4cd_template_status_is_accepted(status)) {
        if (status == 3u) {
            range_mm = 0u;
        } else if ((status == 2u) || (status == 4u) || (status == 12u)) {
            range_mm = max_range_mm;
        }
    }

    printf("  [0x%02X] range=%u status=%u (%s) signal=%u ambient=%u sigma=%u\r\n",
           dev_address,
           vl53l4cd_template_convert_range(range_mm, units),
           status,
           vl53l4cd_template_status_text(status),
           signal_kcps,
           ambient_kcps,
           sigma_mm);
}

void vl53l4cd_template_demo_run(
    vl53l4cd_template_t *ctx,
    uint8_t dev_address,
    uint8_t units,
    uint16_t period_ms,
    uint16_t max_samples
)
{
    uint16_t i;

    if ((ctx == NULL) || (max_samples == 0u)) {
        return;
    }

    /* One-shot sample */
    {
        vl53l4cd_measurement_t m;
        uint16_t attempts = 0u;
        if (vl53l4cd_template_get_measurement(ctx, dev_address, units, &m, &attempts) == VL53L4CD_OK) {
            printf("[one-shot] range=%u status=%u signal=%u ambient=%u sigma=%u attempts=%u\r\n",
                   vl53l4cd_template_convert_range(m.range_mm, units),
                   m.status,
                   m.signal_kcps,
                   m.ambient_kcps,
                   m.sigma_mm,
                   attempts);
        } else {
            printf("[one-shot] failed\r\n");
        }
    }

    /* Continuous sample loop */
    for (i = 0u; i < max_samples; i++) {
        vl53l4cd_measurement_t m;
        uint16_t attempts = 0u;
        if (vl53l4cd_template_get_measurement(ctx, dev_address, units, &m, &attempts) == VL53L4CD_OK) {
            printf("[continuous %u/%u] range=%u status=%u signal=%u ambient=%u sigma=%u attempts=%u\r\n",
                   (unsigned int)(i + 1u),
                   (unsigned int)max_samples,
                   vl53l4cd_template_convert_range(m.range_mm, units),
                   m.status,
                   m.signal_kcps,
                   m.ambient_kcps,
                   m.sigma_mm,
                   attempts);
        } else {
            printf("[continuous %u/%u] failed\r\n",
                   (unsigned int)(i + 1u),
                   (unsigned int)max_samples);
        }

        if (ctx->cfg.delay_ms != NULL) {
            ctx->cfg.delay_ms(period_ms, ctx->cfg.user_ctx);
        }
    }
}

void vl53l4cd_template_demo_run_all(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    uint16_t period_ms,
    uint16_t max_cycles
)
{
    uint16_t cycle;
    vl53l4cd_measurement_t measurements[16];
    uint16_t attempts[16];
    vl53l4cd_status_t statuses[16];

    if ((ctx == NULL) || (dev_addresses == NULL) || (dev_count == 0u) || (max_cycles == 0u) || (dev_count > 16u)) {
        return;
    }

    for (cycle = 0u; cycle < max_cycles; cycle++) {
        uint8_t index;

        printf("[range-all cycle %u/%u]\r\n",
               (unsigned int)(cycle + 1u),
               (unsigned int)max_cycles);

        (void)vl53l4cd_template_get_measurements(
            ctx,
            dev_addresses,
            dev_count,
            units,
            measurements,
            attempts,
            statuses
        );

        for (index = 0u; index < dev_count; index++) {
            if (statuses[index] == VL53L4CD_OK) {
                printf("  [0x%02X] range=%u status=%u (%s) signal=%u ambient=%u sigma=%u attempts=%u\r\n",
                       dev_addresses[index],
                       vl53l4cd_template_convert_range(measurements[index].range_mm, units),
                       measurements[index].status,
                       vl53l4cd_template_status_text(measurements[index].status),
                       measurements[index].signal_kcps,
                       measurements[index].ambient_kcps,
                       measurements[index].sigma_mm,
                       attempts[index]);
            } else {
                printf("  [0x%02X] failed (%d)\r\n", dev_addresses[index], (int)statuses[index]);
            }
        }

        if ((ctx->cfg.delay_ms != NULL) && ((cycle + 1u) < max_cycles)) {
            ctx->cfg.delay_ms(period_ms, ctx->cfg.user_ctx);
        }
    }
}

void vl53l4cd_template_demo_run_all_simultaneous(
    vl53l4cd_template_t *ctx,
    const uint8_t *dev_addresses,
    uint8_t dev_count,
    uint8_t units,
    uint16_t wait_ms,
    uint16_t period_ms,
    uint16_t max_cycles
)
{
    uint16_t cycle;

    if ((ctx == NULL) || (dev_addresses == NULL) || (dev_count == 0u) || (max_cycles == 0u)) {
        return;
    }

    if (ctx->cfg.i2c_write == NULL) {
        return;
    }

    for (cycle = 0u; cycle < max_cycles; cycle++) {
        uint8_t index;

        printf("[range-all-sim cycle %u/%u]\r\n",
               (unsigned int)(cycle + 1u),
               (unsigned int)max_cycles);

        for (index = 0u; index < dev_count; index++) {
            uint8_t tx[2];
            tx[0] = VL53L4CD_CMD_GET_RANGING_RESULT;
            tx[1] = units;

            if (ctx->cfg.i2c_write(dev_addresses[index], tx, (uint8_t)sizeof(tx), ctx->cfg.user_ctx) != 0u) {
                printf("  [0x%02X] start failed\r\n", dev_addresses[index]);
            }
        }

        if ((wait_ms > 0u) && (ctx->cfg.delay_ms != NULL)) {
            ctx->cfg.delay_ms(wait_ms, ctx->cfg.user_ctx);
        }

        for (index = 0u; index < dev_count; index++) {
            uint8_t frame[VL53L4CD_RANGING_FRAME_SIZE];

            if (demo_read_valid_simultaneous_frame(ctx, dev_addresses[index], frame, (uint8_t)sizeof(frame))) {
                demo_print_ranging_frame(
                    dev_addresses[index],
                    units,
                    frame,
                    (uint8_t)sizeof(frame),
                    ctx->cfg.max_range_mm
                );
            } else {
                printf("  [0x%02X] read timeout/fail\r\n", dev_addresses[index]);
            }
        }

        if ((ctx->cfg.delay_ms != NULL) && ((cycle + 1u) < max_cycles) && (period_ms > 0u)) {
            ctx->cfg.delay_ms(period_ms, ctx->cfg.user_ctx);
        }
    }
}
