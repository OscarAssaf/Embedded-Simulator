import pytest
import struct

STATE_IDLE          = 0
STATE_COOLING       = 1
STATE_CRITICAL      = 2
STATE_SENSOR_ERROR  = 3

def parse_status_payload(payload: bytes):
    state, temp, fan_duty = struct.unpack(">BhB", payload)
    return state, temp, fan_duty

def test_normal_operation_fan_off(device):
    device.send_temperature(25)
    msg_type, payload = device.receive_packet()

    assert msg_type == device.MSG_TYPE_STATUS_REPORT
    state, temp, fan_duty = parse_status_payload(payload)

    assert state == STATE_IDLE
    assert temp == 25
    assert fan_duty == 0

def test_cooling_triggered_at_threshold(device):
    device.send_temperature(50)
    msg_type, payload = device.receive_packet()

    assert msg_type == device.MSG_TYPE_STATUS_REPORT
    state, temp, fan_duty = parse_status_payload(payload)

    assert state == STATE_COOLING
    assert temp == 50
    assert fan_duty == 50

def test_hysteresis_cooling_behaviour(device):
    device.send_temperature(48)
    device.receive_packet()

    device.send_temperature(40)
    _, payload = device.receive_packet()
    state, _, fan_duty = parse_status_payload(payload)

    assert state == STATE_COOLING
    assert fan_duty == 50

    device.send_temperature(35)
    _, payload = device.receive_packet()
    state, _, fan_duty = parse_status_payload(payload)

    assert state == STATE_IDLE
    assert fan_duty == 0

def test_critical_overtemperature_shutdown(device):
    device.send_temperature(80)
    msg_type, payload = device.receive_packet()

    state, temp, fan_duty = parse_status_payload(payload)
    assert state == STATE_CRITICAL
    assert fan_duty == 100

def test_fault_injection_corrupt_crc(device):
    device.send_temperature(30, corrupt_crc=True)
    msg_type, payload = device.receive_packet()

    assert msg_type == device.MSG_TYPE_ACK_NACK
    nack_reason = payload[0]
    assert nack_reason == 0x01
