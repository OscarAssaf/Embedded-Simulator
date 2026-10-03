#include "protocol.h"

static protocol_stats_t g_stats;

void protocol_parser_init(void) {
    g_stats.rx_frames_valid = 0;
    g_stats.rx_err_crc = 0;
    g_stats.rx_err_length = 0;
    g_stats.rx_bytes_dropped = 0;
}

const protocol_stats_t* protocol_get_stats(void) {
    return &g_stats;
}

uint16_t crc16_compute(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

typedef enum {
    PARSE_WAIT_SOF,
    PARSE_GET_TYPE,
    PARSE_GET_LEN,
    PARSE_GET_PAYLOAD,
    PARSE_GET_CRC_HIGH,
    PARSE_GET_CRC_LOW
} parse_internal_state_t;

parse_status_t protocol_parse_byte(uint8_t byte, packet_t *out_pkt) {
    static parse_internal_state_t state = PARSE_WAIT_SOF;
    static packet_t current_pkt;
    static uint8_t payload_idx = 0;

    switch (state) {
        case PARSE_WAIT_SOF:
            if (byte == PROTOCOL_SOF) {
                current_pkt.sof = byte;
                state = PARSE_GET_TYPE;
            } else {
                g_stats.rx_bytes_dropped++;
            }
            return PARSE_STATUS_IN_PROGRESS;

        case PARSE_GET_TYPE:
            current_pkt.msg_type = byte;
            state = PARSE_GET_LEN;
            return PARSE_STATUS_IN_PROGRESS;

        case PARSE_GET_LEN:
            if (byte > PROTOCOL_MAX_PAYLOAD) {
                g_stats.rx_err_length++;
                state = PARSE_WAIT_SOF;
                return PARSE_STATUS_ERR_LENGTH;
            }
            current_pkt.length = byte;
            payload_idx = 0;
            if (current_pkt.length == 0) {
                state = PARSE_GET_CRC_HIGH;
            } else {
                state = PARSE_GET_PAYLOAD;
            }
            return PARSE_STATUS_IN_PROGRESS;

        case PARSE_GET_PAYLOAD:
            current_pkt.payload[payload_idx++] = byte;
            if (payload_idx >= current_pkt.length) {
                state = PARSE_GET_CRC_HIGH;
            }
            return PARSE_STATUS_IN_PROGRESS;

        case PARSE_GET_CRC_HIGH:
            current_pkt.crc16 = ((uint16_t)byte) << 8;
            state = PARSE_GET_CRC_LOW;
            return PARSE_STATUS_IN_PROGRESS;

        case PARSE_GET_CRC_LOW:
            current_pkt.crc16 |= (uint16_t)byte;
            state = PARSE_WAIT_SOF;

            uint8_t header_buf[2] = { current_pkt.msg_type, current_pkt.length };
            uint16_t calc_crc = crc16_compute(header_buf, 2);
            for (uint8_t i = 0; i < current_pkt.length; i++) {
                calc_crc ^= (uint16_t)current_pkt.payload[i] << 8;
                for (uint8_t b = 0; b < 8; b++) {
                    if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                    else calc_crc <<= 1;
                }
            }

            if (calc_crc == current_pkt.crc16) {
                g_stats.rx_frames_valid++;
                if (out_pkt) {
                    *out_pkt = current_pkt;
                }
                return PARSE_STATUS_OK_FRAME;
            } else {
                g_stats.rx_err_crc++;
                return PARSE_STATUS_ERR_CRC;
            }
    }

    state = PARSE_WAIT_SOF;
    return PARSE_STATUS_IN_PROGRESS;
}

void protocol_send_nack(uint8_t reason, void (*write_fn)(const uint8_t*, uint16_t)) {
    protocol_send_packet(MSG_TYPE_ACK_NACK, &reason, 1, write_fn);
}

void protocol_send_packet(uint8_t msg_type, const uint8_t *payload, uint8_t length, void (*write_fn)(const uint8_t*, uint16_t)) {
    uint8_t header[3] = { PROTOCOL_SOF, msg_type, length };
    write_fn(header, 3);

    uint8_t crc_calc_buf[2] = { msg_type, length };
    uint16_t crc = crc16_compute(crc_calc_buf, 2);

    if (length > 0 && payload != 0) {
        write_fn(payload, length);
        for (uint8_t i = 0; i < length; i++) {
            crc ^= (uint16_t)payload[i] << 8;
            for (uint8_t b = 0; b < 8; b++) {
                if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
                else crc <<= 1;
            }
        }
    }

    uint8_t crc_bytes[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
    write_fn(crc_bytes, 2);
}
