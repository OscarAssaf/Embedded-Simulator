#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_IDLE = 0,
    STATE_COOLING,
    STATE_CRITICAL,
    STATE_SENSOR_ERROR
} system_state_t;

typedef struct {
    int16_t current_temp_c;
    uint8_t fan_duty_cycle;
    system_state_t state;
    uint32_t error_counter;
} system_context_t;

void fsm_init(system_context_t *ctx);
void fsm_process_temperature(system_context_t *ctx, int16_t new_temp);
void fsm_handle_error(system_context_t *ctx);

#endif
