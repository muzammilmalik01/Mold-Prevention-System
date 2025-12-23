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
| **System Health Monitor** | 1 | Checks for sensor drift, wire disconnection, and I2C failures. Ensures data reliability before processing. | ✅ **Complete** |
| **VTT Model Implementation** | 3 | Implements the **VTT Mathematical Model** (C code) to calculate Mold Index (0-6) based on temp/humidity history. | 🟢 **Initial Implementation Done** |
| **Messaging Module** | - | Handles communication protocols for transmitting data to the Server Node/Gateway. | 🟢 **Initial Implementation Done** |
| **Scheduling/Threads** | - | RMS Scheduling, Mutex Locks for resources and Threading to run all 3 Services. | ✅ **Complete** |
| **Sensor Node Setup** | - | Configures the sensor node hardware and initializes all peripherals. | 🟢 **Initial Implementation Done** |

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
│       ├── system_health.h   # (Done) Public Interface
│       ├── vtt_model.c     # (Done) Main Algo for VTT Model
│       ├── vtt_model.h     # (Done) Public Interface of VTT Model
│       ├── messaging_service.c     # (Code Clean-up pending) Main Algo for Messaging Service
│       └── messaging_service.h       # (Done) Public Interface of Messaging Service
```

Made with ❤️ by muzamil.py