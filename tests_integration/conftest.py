import pytest
from device_driver import DeviceDriver

@pytest.fixture(scope="function")
def device():
    driver = DeviceDriver(host="localhost", port=5555)
    driver.connect()
    yield driver
    driver.close()
