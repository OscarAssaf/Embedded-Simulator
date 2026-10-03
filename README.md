# Embedded Thermal ECU Simulator & Automated Test Harness

A bare-metal C firmware application for an ARM Cortex-M3 microcontroller, featuring a Finite State Machine, UART Hardware Abstraction Layer, and a streaming packet parser with CRC16-CCITT integrity as verification.

The project includes a complete **Software-in-the-Loop ** automated test harness built with **Python** and **pytest**, communicating with the virtual microcontroller over a simulated UART-to-TCP socket in **QEMU**. It features unit testing, communication fault injection, and a closed-loop thermal plant model simulating hardware cooling dynamics.

---

## Architecture & System Overview

```text
 +-----------------------------------------------------------------------------------+
 |                             TEST RIG & SIMULATION (Host)                          |
 |                                                                                   |
 |  +--------------------------+          +---------------------------------------+  |
 |  | Python / pytest Harness  |          | Closed-Loop Thermal Plant Model       |  |
 |  | - Functional SIL Tests   |          | - Heat generation simulation          |  |
 |  | - CRC Fault Injection    |          | - Fan dissipation feedback dynamics   |  |
 |  +-------------+------------+          +-------------------+-------------------+  |
 |                |                                           |                      |
 |                +---------------------+---------------------+                      |
 |                                      |                                            |
 |                         TCP Socket (Port 5555)                                    |
 +--------------------------------------|--------------------------------------------+
                                        | (UART0 emulated stream)
 +--------------------------------------|--------------------------------------------+
 |                              QEMU ARM EMULATOR                                    |
 |  Target Board: TI Stellaris LM3S6965 (ARM Cortex-M3 Bare-Metal)                   |
 |                                                                                   |
 |  +-----------------------------------------------------------------------------+  |
 |  | Firmware Execution                                                          |  |
 |  |                                                                             |  |
 |  |   +-------------------+        +-------------------+                        |  |
 |  |   | Register UART HAL | -----> | Byte-Stream Parser|                        |  |
 |  |   | (Polling I/O)     |        | (CRC16-CCITT)     |                        |  |
 |  |   +-------------------+        +---------+---------+                        |  |
 |  |                                          | Valid Frames / Errors            |  |
 |  |                                          v                                  |  |
 |  |                                +-------------------+                        |  |
 |  |                                | Deterministic FSM |                        |  |
 |  |                                | (Fan Duty / State)|                        |  |
 |  |                                +-------------------+                        |  |
 |  +-----------------------------------------------------------------------------+  |
 +-----------------------------------------------------------------------------------+
```

### Key Engineering Highlights
* **Zero Dynamic Allocation:** Designed for safety-critical embedded systems with strictly static memory allocation (`no malloc`).
* **Robust Packet Parsing:** Stream-oriented state machine parser recovering gracefully from noise, malformed frames, length overflows, and bit flips.
* **SIL Test Automation:** Hardware-less automated testing pipeline allowing full regression, fault injection, and physics-based closed-loop validation before physical silicon is available.

---

## Features

1. **Bare-Metal Firmware (C11):**
   * Target: ARM Cortex-M3 (`LM3S6965EVB`).
   * Custom Linker Script (`linker.ld`) mapping Flash (`0x00000000`) and SRAM (`0x20000000`).
   * Custom vector table & startup code (`src/startup.c`) with zeroed BSS and initialized `.data` segments.
   * Memory-mapped register abstraction for UART0 with non-blocking transmit and receive polling.
   * Deterministic FSM managing thermal states (`IDLE`, `COOLING`, `CRITICAL`, `SENSOR_ERROR`) and proportional fan duty cycle.

2. **Binary Wire Protocol:**
   * Frame Format: `[SOF: 0xAA] [MSG_TYPE: 1B] [LEN: 1B] [PAYLOAD: N bytes] [CRC16: 2B]`
   * Checksum: CRC16-CCITT (`Polynomial: 0x1021`, `Init: 0xFFFF`).
   * Bidirectional status reports and explicit NACK error signaling.

3. **Multi-Tier Testing Suite:**
   * **Host-Based C Unit Tests:** Validates framing, corrupted CRC detection, and buffer overflow protection directly on the host machine.
   * **Integration Tests (pytest):** End-to-end telemetry verification and deliberate CRC corruption testing.
   * **Closed-Loop Physical Simulation:** Simulates ambient temperature, continuous heat dissipation, and dynamic fan cooling behavior in a feedback loop.

---

## Repository Structure

```text
.
├── include/
│   ├── config.h            # System thresholds, clock speeds & buffer definitions
│   ├── fsm.h               # Finite State Machine state definitions and API
│   ├── hal_uart.h          # Hardware Abstraction Layer for UART peripherals
│   └── protocol.h          # Framing tokens, packet structures & CRC prototypes
├── src/
│   ├── startup.c           # Vector table, Reset_Handler, SRAM initialization
│   ├── hal_uart.c          # Memory-mapped register driver for LM3S6965 UART0
│   ├── fsm.c               # Thermal control logic and duty cycle transitions
│   ├── protocol.c          # Streaming byte-by-byte parser and CRC16 engine
│   └── main.c              # Application super-loop and packet dispatcher
├── tests/
│   └── test_protocol.c     # Host-based C unit test harness (GCC / Clang)
├── tests_integration/
│   ├── conftest.py         # Pytest fixtures managing connection lifetimes
│   ├── device_driver.py    # Python TCP-to-UART protocol driver with CRC packing
│   ├── test_firmware.py    # SIL functional & fault injection tests
│   └── test_closed_loop.py # Plant model closed-loop physical thermal regulation test
├── linker.ld               # Memory layout and section definitions for Cortex-M3
├── Makefile                # Cross-compilation, linking, and QEMU automation
└── README.md
```

---

## Prerequisites & Toolchain Setup

To build and run this project locally, install the following tools:

### 1. ARM GNU Toolchain
Cross-compiler targeting ARM Cortex-M microcontrollers (`arm-none-eabi-gcc`).
* **Windows (via winget):**
  ```powershell
  winget install Arm.GnuArmEmbeddedToolchain
  ```
* Verify: `arm-none-eabi-gcc --version`

### 2. QEMU (ARM System Emulator)
Provides system-level emulation of the LM3S6965 evaluation board.
* **Windows:** Install via winget or download from [qemu.org](https://www.qemu.org/download/#windows):
  ```powershell
  winget install SoftwareFreedomConservancy.QEMU
  ```


### 3. Host C Compiler & Make
Required for building the firmware and running host unit tests.
* **GNU Make:** `winget install GnuWin32.Make` (or via MSYS2 / MinGW).
* **GCC:** Install via MSYS2 (`winget install MSYS2.MSYS2`) and run `pacman -S mingw-w64-ucrt-x86_64-gcc`.

### 4. Python 3.10+ & Testing Dependencies
Install `pytest`:
```powershell
python -m pip install pytest
```

---

## How to Build and Run

### 1. Compile the Firmware
Compile the bare-metal C source code with make:
```powershell
make
```
This generates `firmware.elf` in the project root and its dependencies.

---

### 2. Run Host-Based C Unit Tests
Verify protocol parsing and CRC error handling without launching the emulator:
```powershell
gcc -Wall -Wextra -Iinclude tests/test_protocol.c src/protocol.c -o test_runner.exe
.\test_runner.exe
```

Expected output:
```text
=== Running protocol and parser unit tests ===
PASS: test_valid_frame
PASS: test_corrupted_crc
PASS: test_length_overflow
PASS: test_recovery_from_garbage
PASS: test_recovery_after_bad_crc
All unit tests passed successfully!
```

---

### 3. Run Automated Integration & SIL Tests (QEMU + Pytest), you need 2 terminals.

The integration test suite connects to QEMU over a local TCP socket serving as the emulated serial link.

#### Step A: Start the QEMU Target (Terminal 1)
```powershell
make run-socket
```
*QEMU initializes the Cortex-M3 core, binds UART0 to `localhost:5555`, and waits for test client connections.*

#### Step B: Execute the Test Suite (Terminal 2)

* **Run all functional and fault-injection tests:**
  ```powershell
  pytest tests_integration/test_firmware.py -v
  ```

* **Run the Closed-Loop Thermal Regulation Simulation:**
  ```powershell
  pytest tests_integration/test_closed_loop.py -v -s
  ```

During the closed-loop run, the Python plant model streams temperatures, inspects firmware fan actuation, and displays dynamic feedback regulation in real-time:

```text
--- Starting closed-loop thermal simulation ---
Cycle 00: Temp =  25.0°C | State: IDLE     | FAN OFF
Cycle 04: Temp =  35.0°C | State: IDLE     | FAN OFF
Cycle 08: Temp =  45.0°C | State: COOLING  | FAN ON [#####     ] 50%
Cycle 12: Temp =  43.5°C | State: COOLING  | FAN ON [#####     ] 50%
Cycle 16: Temp =  42.0°C | State: IDLE     | FAN OFF
...
Result: Max temp = 45.0°C, Final temp = 41.5°C -> Thermal heat regulation successful!
```

---

## Author & Contact

* **Author:** Oscar Assaf
* **Focus:** Embedded Systems, Firmware Development & Automated Test Engineering (SIL / HIL)
