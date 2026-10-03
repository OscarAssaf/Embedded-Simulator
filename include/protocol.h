#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_SOF 0xAA
#define PROTOCOL_MAX_PAYLOAD 32

typedef struct __attribute__((packed)) {
    uint8_t  sof;
    uint8_t  msg_type;
    uint8_t  length;
    uint8_t  payload[PROTOCOL_MAX_PAYLOAD];
    uint16_t crc16;
} packet_t;

uint16_t crc16_compute(const uint8_t *data, uint16_t length);

#endif
