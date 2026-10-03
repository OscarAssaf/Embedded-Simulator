import struct
import pytest
from device_driver import DeviceDriver

def test_closed_loop_thermal_regulation(device):
   
    current_temp = 25.0       # Starting temperature 
    heat_generation = 2.5     # Heat gotten per 'cycle'
    cooling_factor = 6.0      # How much effect the imaginiary fan givevs
    history = []

    print("\n--- Startar sluten termisk simulering ---")

    for cycle in range(35):
        # Sends temp to the component
        device.send_temperature(int(current_temp))

        # 2. retrieves firmware status 
        msg_type, payload = device.receive_packet()
        assert msg_type == DeviceDriver.MSG_TYPE_STATUS_REPORT

        # Payload in C: state (uint8), temp (int16), fan_duty (uint8)
        state, reported_temp, fan_duty = struct.unpack(">BhB", payload)

        history.append((current_temp, fan_duty, state))

        # Visualize the effect in terminal
        fan_bar = "#" * (fan_duty // 10)
        fan_text = f"Fan on [{fan_bar:<10}] {fan_duty}%" if fan_duty > 0 else "Fan Off"
        state_names = {0: "IDLE", 1: "COOLING", 2: "CRITICAL", 3: "ERROR"}
        state_str = state_names.get(state, f"STATE_{state}")
        print(f"Cykel {cycle:02d}: Temp = {current_temp:5.1f}°C | State: {state_str:<8} | {fan_text}")

        # 3. Cooling effect for the next.
        fan_cooling = (fan_duty / 100.0) * cooling_factor
        current_temp = current_temp + heat_generation - fan_cooling

    # 4. VErifying
    max_temp = max(h[0] for h in history)
    final_temp = history[-1][0]

    assert max_temp >= 45.0, "Temperature never reached cooling threshold"
    assert max_temp < 75.0, "Fan failed to cool system, reached critical temperature!"
    assert final_temp < max_temp, "Temperature did not decrease after fan actuation"
    print(f"\nResult: Max temp = {max_temp:.1f}°C, Final temp = {final_temp:.1f}°C -> Thermal regulation successful!")
