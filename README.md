# 🐝 **StingSense: Buzzing into the Future of Bus Monitoring** 🐝

Welcome to the hive of innovation! At Georgia Tech, we're revolutionizing campus transportation with **StingSense**, an IoT-powered project that tracks and analyzes the behavior of Georgia Tech's Stinger buses. This README outlines the technical details of our project, built on the Actinius Icarus board and Zephyr RTOS.

## 🎓 **About the Project**

This repository contains code for the CS 8903 Special Problems class. Using Zephyr RTOS 2.7.x and Nordic's nRF9160-based Actinius Icarus board, **StingSense** collects and processes real-time data from Stinger buses, including acceleration, location, and time synchronization. The goal is to enhance bus monitoring and provide actionable insights.

## 🌟 **Why StingSense?**

- **Sting**: Inspired by Georgia Tech's iconic Stinger buses.
- **Sense**: Enabling smart sensing capabilities for acceleration, GPS location, and more.

## 🚀 **Features**

- **Accelerometer Data Collection**: Captures real-time acceleration data along X, Y, Z axes to monitor bus movement dynamics.
- **GPS Location Tracking**: Provides precise latitude, longitude, and altitude data for bus location tracking.
- **Real-Time Clock (RTC)**: Synchronizes timestamps in EST for accurate data logging.
- **LTE Connectivity**: Sends collected data to cloud servers using cellular connectivity.
- **JSON Formatting**: Organizes sensor data into structured JSON for easy parsing and analysis.
- **Periodic Reporting**: Automatically sends sensor data every 3 seconds for continuous monitoring.

## 🛠️ **Hardware Requirements**

- **Actinius Icarus Board**: IoT development platform powered by Nordic's nRF9160 SiP with LTE-M/NB-IoT and GPS capabilities.
- **GPS Antenna**: Required for accurate location tracking.
- **LTE Antenna**: Ensures reliable cellular connectivity for data transmission.
- **USB Cable**: Facilitates power supply and debugging.

## 💾 **Software Requirements**

- **Zephyr RTOS 2.7.x**: Lightweight real-time operating system optimized for embedded systems.
- **Nordic Connect SDK (NCS)**: Provides libraries and tools compatible with Zephyr RTOS for nRF9160 development.
- **West Build Tools**: Command-line interface for building, flashing, and managing Zephyr projects.

## 📂 **Project Structure**

```plaintext
bus-rtos/
├── CMakeLists.txt        # Build system configuration
├── prj.conf              # Project-specific configuration
├── overlay-pgps.conf     # Assisted GPS configuration
├── overlay-supl.conf     # SUPL (Secure User Plane Location) configuration
├── Kconfig               # Kernel configuration options
├── sample.yaml           # Sample YAML configuration file
└── src/
    ├── main.c            # Main application logic
    ├── accelerometer.c/h # Accelerometer driver code
    ├── rtc.c/h           # Real-Time Clock handling
    ├── startup.c         # Modem initialization routines
    ├── assistance/       # Assisted GPS utilities
    ├── mcc_location/     # Mobile Country Code-based location utilities
    └── factory_almanac/  # Preloaded GPS almanac files
└── samples/
    ├── accelerometer/    # Accelerometer test examples
    ├── date-time/        # RTC test examples
    └── gps/              # GPS test examples
└── boards/
    ├── actinius_icarus_ns.overlay  # Custom board overlay for Actinius Icarus
    └── circuitdojo_feather_nrf9160_ns.conf  # Configuration for Circuit Dojo Feather board (alternative)
```

## 🛠️ **Building and Flashing**

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/kpath1999/bus-rtos.git
   cd bus-rtos
   ```

2. **Build the Project**:
   ```bash
   west build -b actinius_icarus_ns -p always
   ```

3. **Flash the Firmware**:
   - Locate `app_update.bin` in `build/zephyr/`.
   - Upload it via the Actinius portal or flash directly using USB connection.

## 📊 **Data Flow**

![Data Flow Diagram](data_flow_diagram: Sensor data flows from buses to cloud servers via LTE connectivity.)

## 📝 **Sample Output**

When StingSense is operational, it generates output similar to this:

```
Sensor Data
-------------------------------------------------------------------------------
Date/Time: 2025-03-17 14:55:00 EST
Acceleration: X: -0.153228 (m/s^2), Y: 7.048488 (m/s^2), Z: 6.435576 (m/s^2)
GPS: Lat: 42.360100, Lon: -71.058900, Alt: 10.500000
-------------------------------------------------------------------------------
JSON Data: {"datetime":"2025-03-17 14:55:00 EST","latitude":42.3601,"longitude":-71.0589,"altitude":10.5,"x_accel":-0.153228,"y_accel":7.048488,"z_accel":6.435576}
-------------------------------------------------------------------------------
Data successfully sent over LTE.
```

## 📝 **Implementation Notes**

- **AT Commands**: Used for modem communication and LTE/GPS configuration.
- **GPS Fix Delay**: Initial GPS fix may take a few minutes; ensure clear sky visibility.
- **LTE Connectivity**: Requires a valid SIM card with network coverage supporting LTE-M/NB-IoT.
- **Power Optimization**: Implemented low-power modes to maximize battery efficiency.

## 🙏 **Acknowledgments**

- **Zephyr RTOS**: For providing a robust foundation for embedded development.
- **Actinius**: For their powerful Icarus IoT development board.
- **Prof. Ashutosh Dhekne**: For guiding this innovative project on IoT-enabled bus monitoring.

Buzz into action with StingSense! 🚍🐝