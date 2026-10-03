import socket
import struct
import time
from typing import Optional, Tuple


class DeviceDriver:
    SOF = 0xAA
    MSG_TYPE_TEMP_TELEMETRY = 0x01
    MSG_TYPE_STATUS_REPORT  = 0x02
    MSG_TYPE_ACK_NACK       = 0x03

    def __init__(self, host: str = "127.0.0.1", port: int = 5555, timeout: float = 2.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None

    def connect(self, retries: int = 10, delay: float = 0.5):
        for _ in range(retries):
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(self.timeout)
                self.sock.connect((self.host, self.port))
                return
            except (ConnectionRefusedError, OSError):
                time.sleep(delay)
        raise ConnectionError(f"Kunde inte ansluta till QEMU pa {self.host}:{self.port}")

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def _recv_exact(self, num_bytes: int) -> bytes:
        data = bytearray()
        while len(data) < num_bytes:
            chunk = self.sock.recv(num_bytes - len(data))
            if not chunk:
                raise ConnectionResetError("Anslutningen stangdes av enheten")
            data.extend(chunk)
        return bytes(data)

    @staticmethod
    def compute_crc16(data: bytes) -> int:
        crc = 0xFFFF
        for byte in data:
            crc ^= (byte << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x1021) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF
        return crc

    def send_temperature(self, temp_celsius: int, corrupt_crc: bool = False) -> None:
        payload = struct.pack(">h", temp_celsius)
        msg_type = self.MSG_TYPE_TEMP_TELEMETRY
        length = len(payload)

        crc_data = bytes([msg_type, length]) + payload
        crc = self.compute_crc16(crc_data)

        if corrupt_crc:
            crc ^= 0xFFFF

        packet = struct.pack(">BBB", self.SOF, msg_type, length) + payload + struct.pack(">H", crc)
        self.sock.sendall(packet)

    def receive_packet(self) -> Tuple[int, bytes]:
        while True:
            b = self._recv_exact(1)
            if b[0] == self.SOF:
                break

        header = self._recv_exact(2)
        msg_type, length = struct.unpack(">BB", header)

        payload = b""
        if length > 0:
            payload = self._recv_exact(length)

        
        crc_bytes = self._recv_exact(2)
        received_crc = struct.unpack(">H", crc_bytes)[0]

        expected_crc = self.compute_crc16(header + payload)
        if received_crc != expected_crc:
            raise ValueError(f"CRC Mismatch! Mottagen: {hex(received_crc)}, Beraknad: {hex(expected_crc)}")

        return msg_type, payload
