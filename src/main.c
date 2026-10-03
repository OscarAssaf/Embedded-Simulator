#include "hal_uart.h"
#include "protocol.h"
#include "fsm.h"

int main(void) {
    hal_uart_init();
    protocol_parser_init();

    system_context_t sys_ctx;
    fsm_init(&sys_ctx);

    packet_t rx_packet;
    uint8_t in_byte;

    while (1) {
        while (hal_uart_getc(&in_byte)) {
            parse_status_t status = protocol_parse_byte(in_byte, &rx_packet);

            if (status == PARSE_STATUS_OK_FRAME) {
                if (rx_packet.msg_type == MSG_TYPE_TEMP_TELEMETRY && rx_packet.length == 2) {
                    int16_t received_temp = (int16_t)((rx_packet.payload[0] << 8) | rx_packet.payload[1]);
                    fsm_process_temperature(&sys_ctx, received_temp);

                    uint8_t status_payload[4];
                    status_payload[0] = (uint8_t)sys_ctx.state;
                    status_payload[1] = (uint8_t)((sys_ctx.current_temp_c >> 8) & 0xFF);
                    status_payload[2] = (uint8_t)(sys_ctx.current_temp_c & 0xFF);
                    status_payload[3] = sys_ctx.fan_duty_cycle;

                    protocol_send_packet(MSG_TYPE_STATUS_REPORT, status_payload, 4, hal_uart_write);
                }
            } else if (status == PARSE_STATUS_ERR_CRC) {
                fsm_handle_error(&sys_ctx);
                protocol_send_nack(NACK_INVALID_CRC, hal_uart_write);
            } else if (status == PARSE_STATUS_ERR_LENGTH) {
                fsm_handle_error(&sys_ctx);
                protocol_send_nack(NACK_PAYLOAD_TOO_BIG, hal_uart_write);
            }
        }
    }

    return 0;
}
