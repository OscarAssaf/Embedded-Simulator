#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "protocol.h"

static parse_status_t feed_bytes(const uint8_t *data, size_t len, packet_t *out_pkt) {
    parse_status_t last_status = PARSE_STATUS_IN_PROGRESS;
    for (size_t i = 0; i < len; i++) {
        last_status = protocol_parse_byte(data[i], out_pkt);
    }
    return last_status;
}

void test_valid_frame(void) {
    protocol_parser_init();
    packet_t pkt;
    uint8_t raw[] = { 0xAA, 0x01, 0x02, 0x00, 0x32, 0x8A, 0x05 };
    
    parse_status_t status = feed_bytes(raw, sizeof(raw), &pkt);
    
    assert(status == PARSE_STATUS_OK_FRAME);
    assert(pkt.msg_type == 0x01);
    assert(pkt.length == 2);
    assert(pkt.payload[0] == 0x00 && pkt.payload[1] == 0x32);
    assert(protocol_get_stats()->rx_frames_valid == 1);
    printf("PASS: test_valid_frame\n");
}

void test_corrupted_crc(void) {
    protocol_parser_init();
    packet_t pkt;
    uint8_t corrupted[] = { 0xAA, 0x01, 0x02, 0x00, 0x32, 0x8A, 0xFF };

    parse_status_t status = feed_bytes(corrupted, sizeof(corrupted), &pkt);

    assert(status == PARSE_STATUS_ERR_CRC);
    assert(protocol_get_stats()->rx_err_crc == 1);
    printf("PASS: test_corrupted_crc\n");
}

void test_length_overflow(void) {
    protocol_parser_init();
    packet_t pkt;
    uint8_t invalid_len[] = { 0xAA, 0x01, 50 };

    parse_status_t status = feed_bytes(invalid_len, sizeof(invalid_len), &pkt);

    assert(status == PARSE_STATUS_ERR_LENGTH);
    assert(protocol_get_stats()->rx_err_length == 1);
    printf("PASS: test_length_overflow\n");
}

void test_recovery_from_garbage(void) {
    protocol_parser_init();
    packet_t pkt;
    uint8_t stream[] = {
        0x12, 0xFF, 0x00, 0x55,
        0xAA, 0x01, 0x02, 0x00, 0x32, 0x8A, 0x05
    };

    parse_status_t status = feed_bytes(stream, sizeof(stream), &pkt);

    assert(status == PARSE_STATUS_OK_FRAME);
    assert(protocol_get_stats()->rx_bytes_dropped == 4);
    assert(protocol_get_stats()->rx_frames_valid == 1);
    printf("PASS: test_recovery_from_garbage\n");
}

void test_recovery_after_bad_crc(void) {
    protocol_parser_init();
    packet_t pkt;
    uint8_t bad_frame[]   = { 0xAA, 0x01, 0x02, 0x00, 0x32, 0x00, 0x00 };
    uint8_t valid_frame[] = { 0xAA, 0x01, 0x02, 0x00, 0x32, 0x8A, 0x05 };

    parse_status_t status1 = feed_bytes(bad_frame, sizeof(bad_frame), &pkt);
    assert(status1 == PARSE_STATUS_ERR_CRC);

    parse_status_t status2 = feed_bytes(valid_frame, sizeof(valid_frame), &pkt);
    assert(status2 == PARSE_STATUS_OK_FRAME);

    assert(protocol_get_stats()->rx_err_crc == 1);
    assert(protocol_get_stats()->rx_frames_valid == 1);
    printf("PASS: test_recovery_after_bad_crc\n");
}

int main(void) {
    printf("=== Korande enhetstester for protokoll och parser ===\n");
    test_valid_frame();
    test_corrupted_crc();
    test_length_overflow();
    test_recovery_from_garbage();
    test_recovery_after_bad_crc();
    printf("Alla tester lyckades!\n");
    return 0;
}
