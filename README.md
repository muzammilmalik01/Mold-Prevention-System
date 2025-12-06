# AERIS - Indoor Air Quality Monitoring System (Embedded Firmware)

**Firmware for the Nordic nRF52840 Sensor Nodes**

This repository contains the embedded firmware for the **AERIS** system, a Real-Time IoT solution designed to predict and prevent indoor mold growth. The system uses dual-sensor redundancy and the **VTT Mold Index model** (Edge Computing) to detect risks before they become visible.

## 🏗️ System Architecture

The firmware is built on **Zephyr RTOS** and follows a modular microservice architecture. Each core function is isolated into its own service module for reliability and maintainability.

### Hardware Stack

* **MCU:** Nordic nRF52840 (Cortex-M4F)
* **Sensors:** 2x DHT20 Temperature & Humidity Sensors (I2C)
* **Network:** OpenThread Mesh
* **OS:** Zephyr RTOS

## 🛠️ Microservices Status

Below is the implementation status of the core embedded services defined in the system architecture.

| Service Name | Priority | Description | Status |
| :--- | :---: | :--- | :---: |
| **System Health Monitor** | 3 | Checks for sensor drift, wire disconnection, and I2C failures. Ensures data reliability before processing. | ✅ **Complete** |
| **Sensor Data Collection** | 2 | Reads raw temperature and humidity from dual DHT20 sensors via I2C. | 🚧 **In Progress** |
| **Mold Risk Detection** | 2 | Implements the **VTT Mathematical Model** (C code) to calculate Mold Index (0-6) based on temp/humidity history. | ⏳ **Pending** |
| **Alert Generation** | 1 | Triggers immediate alerts if Mold Index > 1.0 or if critical hardware failure occurs (Hard Real-Time). | ⏳ **Pending** |
| **Thread Communication** | 5 | Transmits JSON payloads via OpenThread Mesh to the Server Node/Gateway. | ⏳ **Pending** |

## 📂 Project Structure

The project follows a modular C structure:

```text
aeris_firmware/
├── app.overlay          # Device Tree (I2C Pin Definitions)
├── CMakeLists.txt       # Build Configuration
├── prj.conf             # Zephyr Kernel Config (Logging, I2C, FPU)
├── src/
│   ├── main.c           # Scheduler & Main Loop
│   └── modules/         # Independent Microservices
│       ├── system_health.c   # (Done) Health Logic & Drift Check
│       ├── system_health.h   # Public Interface
│       ├── sensor_task.c     # (Next) I2C Driver Handling
│       └── vtt_model.c       # (Future) Mold Index Math
## 🚀 Getting Started

**Build:**  
```bash
west build -b nrf52840dk_nrf52840
west flashnrf
nrfjprog --rtt
```

Made with ❤️ by muzamil.py