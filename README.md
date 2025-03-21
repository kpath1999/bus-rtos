# Firmware Code for StingSense Project

This is where my code will be for the CS 8903 Special Problems class with Professor Ashutosh Dhekne. We're using an Actinius Icarus board for determining Georgia Tech bus behavior across different times of the day. The firmware is created using Zephyr RTOS 2.7.x. Done right, it would integrate multiple sensors (accelerometer, GPS) with real-time clock and LTE connectivity to collect, format, and transmit the bus data.

We call this project _StingSense_ - the 'Sting' comes from the Georgia Tech Stinger buses, and 'Sense' comes from the inertial sensing part of our set-up.

## Features

- **Accelerometer Data Collection**: Captures 3-axis acceleration measurements (X, Y, Z) using the onboard LIS2DH sensor
- **GPS Location Tracking**: Retrieves latitude, longitude, and altitude data via the nRF9160 modem
- **Real-Time Clock**: Provides accurate timestamping of sensor readings in EST timezone
- **LTE Connectivity**: Transmits collected data over cellular networks using the nRF9160 modem
- **JSON Formatting**: Structures sensor data in a standardized JSON format for easy processing
- **Periodic Reporting**: Configurable data collection and transmission interval (default: every 3 seconds)

## Hardware Requirements

- Actinius Icarus Board (nRF9160-based)
- GPS Antenna (connected to GPS port)
- LTE Antenna (connected to LTE port)
- USB connection for power and debugging

## Software Requirements

- Zephyr RTOS 2.7.x
- Nordic Connect SDK compatible with Zephyr 2.7.x
- West build tools

## Project Structure

```
bus-rtos/
├── CMakeLists.txt
├── prj.conf
└── src/
    ├── main.c               # Single unified main()
    ├── accelerometer.c/h    
    ├── rtc.c/h              
    ├── startup.c            # Modem initialization code only
    ├── gps_helper.c/h       # GPS functionality extracted from Nordic sample
```

## Building and Flashing

1. Clone this repository:
   ```bash
   git clone https://github.com/kpath1999/bus-rtos.git
   cd bus-rtos
   ```

2. Build the project:
   ```bash
   west build -b actinius_icarus_ns -p always
   ```

3. Flash to your Actinius Icarus board:
   - Locate the `app_update.bin` file in the `build/zephyr/` directory
   - Upload this file to the Actinius portal
   - Connect your Icarus board via USB and flash the application

## Data Format

The application collects and transmits data in the following JSON format:

```json
{
  "datetime": "2025-03-17 14:55:00 EST",
  "latitude": 42.3601,
  "longitude": -71.0589,
  "altitude": 10.5,
  "x_accel": -0.153228,
  "y_accel": 7.048488,
  "z_accel": 6.435576
}
```

## Sample Output

When running, the application outputs formatted sensor data to the serial terminal:

```
Sensor Data
-------------------------------------------------------------------------------
Date/Time: 2025-03-17 14:55:00 EST
Acceleration: X: -0.153228 (m/s^2), Y: 7.048488 (m/s^2), Z: 6.435576 (m/s^2)
GPS: Lat: 42.360100, Lon: -71.058900, Alt: 10.500000
-------------------------------------------------------------------------------
JSON Data: {"datetime":"2025-03-17 14:55:00 EST","latitude":42.3601,"longitude":-71.0589,"altitude":10.5,"x_accel":-0.153228,"y_accel":7.048488,"z_accel":6.435576}
-------------------------------------------------------------------------------
Data successfully sent over LTE
```

## Implementation Notes

- The project uses AT commands to interface with the nRF9160 modem for GPS and LTE functionality
- GPS may require several minutes to acquire a fix when used outdoors
- LTE connectivity requires proper SIM configuration and network coverage
- Power management is optimized for periodic data collection and transmission

## Configuration Options

Key configuration options in `prj.conf`:

```conf
# Sensor sampling rate
CONFIG_LIS2DH_ODR_2=y         # Accelerometer sampling rate

# LTE & GPS
CONFIG_MODEM=y                # Enable modem functionality
CONFIG_AT_CMD=y               # Enable AT commands for modem control

# Data transmission interval
# Modify k_sleep(K_SECONDS(3)) in main.c to change the interval
```

## License

This project is licensed under the BSD-3-Clause License - see the LICENSE file for details.

## Acknowledgments

- Based on Zephyr RTOS sample applications
- Utilizes Actinius Icarus board capabilities
- Inspired by Prof. Dhekne's idea of monitoring bus fleet using IoT technology