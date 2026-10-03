#include "fsm.h"
#include "config.h"

void fsm_init(system_context_t *ctx) {
    if (!ctx) return;
    ctx->current_temp_c = 25;
    ctx->fan_duty_cycle = 0;
    ctx->state = STATE_IDLE;
    ctx->error_counter = 0;
}

void fsm_process_temperature(system_context_t *ctx, int16_t new_temp) {
    if (!ctx) return;
    ctx->current_temp_c = new_temp;

    switch (ctx->state) {
        case STATE_IDLE:
            if (new_temp >= TEMP_THRESHOLD_CRITICAL) {
                ctx->state = STATE_CRITICAL;
                ctx->fan_duty_cycle = 100;
            } else if (new_temp >= TEMP_THRESHOLD_COOLING) {
                ctx->state = STATE_COOLING;
                ctx->fan_duty_cycle = 50;
            }
            break;

        case STATE_COOLING:
            if (new_temp >= TEMP_THRESHOLD_CRITICAL) {
                ctx->state = STATE_CRITICAL;
                ctx->fan_duty_cycle = 100;
            } else if (new_temp <= TEMP_THRESHOLD_HYSTERESIS) {
                ctx->state = STATE_IDLE;
                ctx->fan_duty_cycle = 0;
            }
            break;

        case STATE_CRITICAL:
            if (new_temp < TEMP_THRESHOLD_COOLING) {
                ctx->state = STATE_COOLING;
                ctx->fan_duty_cycle = 50;
            }
            break;

        case STATE_SENSOR_ERROR:
            ctx->fan_duty_cycle = 100;
            break;
    }
}

void fsm_handle_error(system_context_t *ctx) {
    if (!ctx) return;
    ctx->error_counter++;
    ctx->state = STATE_SENSOR_ERROR;
    ctx->fan_duty_cycle = 100;
}
