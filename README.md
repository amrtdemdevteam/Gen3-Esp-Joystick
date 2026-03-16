# esp_joy_stick

ROS2 node for reading joystick data from an ESP32 (Bluepad32 firmware) over USB serial.

## Repository Structure

```
esp_joy_stick/
├── bluepad32/                   # Arduino firmware for ESP32 (Bluepad32-based)
├── esp_joystick_interfaces/     # ROS2 package: custom message definitions
│   └── msg/JoystickInfo.msg
├── esp_joystick_ros2/           # ROS2 package: serial reader node
│   ├── config/esp_joystick_params.yaml
│   ├── launch/esp_joystick.launch.py
│   ├── include/esp_joystick_ros2/
│   │   ├── esp_joystick_node.hpp
│   │   └── serial_reader.hpp
│   └── src/ + utils/
└── test_serial.py               # Standalone serial debug utility
```

## Dependencies

- ROS2 Humble
- [`serial`](https://github.com/RoverRobotics-forks/serial-ros2) — build from source and place in workspace `src/`

```bash
cd <ws>/src
git clone https://github.com/RoverRobotics-forks/serial-ros2.git serial
```

## Build

```bash
colcon build --packages-select esp_joystick_interfaces esp_joystick_ros2
source install/setup.bash
```

## Usage

```bash
# Default params (ttyUSB0, 115200 baud)
ros2 launch esp_joystick_ros2 esp_joystick.launch.py

# Custom params file
ros2 launch esp_joystick_ros2 esp_joystick.launch.py params_file:=/path/to/params.yaml
```

Edit `config/esp_joystick_params.yaml` to change serial port or other settings.

## Topic

| Topic  | Type                                       | QoS                         |
| ------ | ------------------------------------------ | --------------------------- |
| `/joy` | `esp_joystick_interfaces/msg/JoystickInfo` | SensorDataQoS (best-effort) |

### JoystickInfo fields

```
std_msgs/Header header          # frame_id: "joy_link"

# Buttons (uint8, 0 or 1)
uint8 a, b, x, y
uint8 l1, l2, r1, r2
uint8 axis_l, axis_r
uint8 up, down, left, right     # D-pad
uint8 select, start

# Analog sticks (raw int16 from ESP32)
int16 lx, ly, rx, ry
```

## Parameters

| Parameter                | Default        | Description                       |
| ------------------------ | -------------- | --------------------------------- |
| `serial_port`            | `/dev/ttyUSB0` | Serial device path                |
| `baud_rate`              | `115200`       | Baud rate                         |
| `crc_validation_enabled` | `true`         | XOR CRC packet validation         |
| `reconnect_interval_ms`  | `2000`         | Retry interval on disconnect (ms) |

## Serial Packet Format

```
15 bytes: [0xAA | len(12) | buttons(u16) | misc(u8) | dpad(u8) | lx(i16) | ly(i16) | rx(i16) | ry(i16) | CRC(u8)]
```

CRC = XOR of bytes 0–13.

**Button bit mapping (from `buttons` u16):**
A=0, B=1, X=2, Y=3, L1=4, R1=5, L2=6, R2=7, axis_l=8, axis_r=9

**D-pad bit mapping (from `dpad` u8):**
up=0, down=1, right=2, left=3

**Misc bit mapping:**
select=bit1, start=bit2
