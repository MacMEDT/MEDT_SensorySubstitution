from smbus2 import SMBus
import time
import random

I2C_BUS = 1
ESP32_ADDR = 0x28 #slave addresss

ROWS, COLS = 5, 5
N = ROWS * COLS

def checksum_xor(bytes_list):
    cs = 0
    for b in bytes_list:
        cs ^= (b & 0xFF)
    return cs

def send_frame(bus, mode, grid25):
    """Sends: [mode][25 bytes][checksum]"""
    assert len(grid25) == N
    
    payload = [mode] + grid25
    cs = checksum_xor(payload)
    payload.append(cs)

    command = payload[0]
    data = payload[1:]
    bus.write_i2c_block_data(ESP32_ADDR, command, data)

def demo_pattern(mode):
    #random grid for testing
    return [random.randint(0, 255) for _ in range(N)]

def main():
    #send frame via I2C to ESP32
    with SMBus(I2C_BUS) as bus:
        mode = 1
        while True:
            grid25 = demo_pattern(mode)
            send_frame(bus, mode, grid25)

            time.sleep(1/30)

if __name__ == "__main__":
    main()

