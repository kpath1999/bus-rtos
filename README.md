# 🐝 **StingSense: Buzzing into the Future of Bus Monitoring** 🐝

Welcome to the hive of innovation! Here at Georgia Tech, we're not just riding the bus; we're riding the wave of IoT technology with our **StingSense** project. This is where the rubber meets the road, or rather, where the code meets the bus!

## 🎓 **About the Project**

This repository is the buzzing heart of our CS 8903 Special Problems class, where we're using the Actinius Icarus board to track and analyze the behavior of Georgia Tech's Stinger buses. With Zephyr RTOS 2.7.x, we're turning these buses into smart, data-collecting machines that can tell us more than just when they're late!

## 🌟 **Why StingSense?**

- **Sting**: Because we're all about the Georgia Tech Stinger buses!
- **Sense**: Because we're sensing everything from acceleration to location, making our buses smarter than ever.

## 🚀 **Features**

- **Accelerometer Data Collection**: Feel the G-forces as the bus navigates the campus. X, Y, Z, we've got it all!
- **GPS Location Tracking**: Know exactly where your bus is, down to the last decimal degree. No more guessing games!
- **Real-Time Clock**: Time is of the essence, and we've got it down to the second in EST.
- **LTE Connectivity**: Our buses are now connected, sending their data to the cloud faster than you can say "buzz off!"
- **JSON Formatting**: Data so neat, it's like bees organizing their hive.
- **Periodic Reporting**: Every 3 seconds, we get a fresh batch of data. It's like a bus schedule, but for data!

## 🛠️ **Hardware Requirements**

- **Actinius Icarus Board**: The brain of our operation, powered by nRF9160.
- **GPS Antenna**: To keep our buses on track.
- **LTE Antenna**: For that sweet, sweet connectivity.
- **USB Connection**: For power and debugging, because even bees need a recharge.

## 💾 **Software Requirements**

- **Zephyr RTOS 2.7.x**: The operating system that keeps our project buzzing.
- **Nordic Connect SDK**: Compatible with Zephyr, because we like to play nice with others.
- **West Build Tools**: Our trusty tool for building and flashing.

## 📂 **Project Structure**

```plaintext
bus-rtos/
├── CMakeLists.txt       # The blueprint of our hive
├── prj.conf             # Configuration, because even bees need rules
└── src/
    ├── main.c           # The queen bee of our project
    ├── accelerometer.c/h # Acceleration, acceleration, acceleration!
    ├── rtc.c/h           # Timekeeping, because punctuality matters
    ├── startup.c         # Modem initialization, our bus's wake-up call
    └── gps_helper.c/h    # GPS, because we know where we're going
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
   - Find `app_update.bin` in `build/zephyr/`
   - Upload to the Actinius portal
   - Connect your Icarus board via USB and flash the app

## 📊 **Data Flow**

![Data Flow Diagram](data_flow_diagram: our data buzzes from the bus to the server!)

## 📝 **Sample Output**

When our StingSense is running, here's what you'll see:

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

## 📝 **Implementation Notes**

- **AT Commands**: We're talking to the modem like it's a beekeeper.
- **GPS Fix**: It might take a few minutes to get a fix, but patience is a virtue.
- **LTE**: Proper SIM and network coverage are key to our hive's communication.
- **Power Management**: We're optimizing for efficiency, because even bees need to conserve energy.

## 🙏 **Acknowledgments**

- **Zephyr RTOS**: For providing the foundation of our project.
- **Actinius**: For the Icarus board, our hardware hive.
- **Prof. Ashutosh Dhekne**: For the brilliant idea of monitoring bus behavior with IoT.