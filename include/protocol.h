#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_SOF 0xAA
#define PROTOCOL_MAX_PAYLOAD 32

typedef enum {
    PARSE_STATUS_IN_PROGRESS = 0,
    PARSE_STATUS_OK_FRAME,
    PARSE_STATUS_ERR_CRC,
    PARSE_STATUS_ERR_LENGTH
} parse_status_t;

typedef struct __attribute__((packed)) {
    uint8_t  sof;
    uint8_t  msg_type;
    uint8_t  length;
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD];
    uint16_t crc16;
} packet_t;

typedef struct {
    uint32_t rx_frames_valid;
    uint32_t rx_err_crc;
    uint32_t rx_err_length;
    uint32_t rx_bytes_dropped;
} protocol_stats_t;

uint16_t crc16_compute(const uint8_t *data, uint16_t length);
void protocol_parser_init(void);
parse_status_t protocol_parse_byte(uint8_t byte, packet_t *out_pkt);
const protocol_stats_t* protocol_get_stats(void);

#endif
