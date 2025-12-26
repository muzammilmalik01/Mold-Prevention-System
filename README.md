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
| **(Sensor Node) System Health Monitor** | 1 | Checks for sensor drift, wire disconnection, and I2C failures. Ensures data reliability before processing. | ✅ **Complete** |
| **(Sensor Node) VTT Model Implementation** | 3 | Implements the **VTT Mathematical Model** (C code) to calculate Mold Index (0-6) based on temp/humidity history. | 🟡 **Testing, Optimization & Validation** |
| **(Sensor Node) Messaging Module** | - | Handles communication protocols for transmitting data to the Server Node/Gateway. | ✅ **Done** |
| **(Sensor Node) Scheduling/Threads** | - | RMS Scheduling, Mutex Locks for resources and Threading to run all 3 Services. | ✅ **Complete** |
| **Server Node Setup** | - | Configures the sensor node hardware and initializes all peripherals. | ✅ **Complete** |
| **(Server Node) Network Listener** | 1 | Listens to CoAP Service, Updates the Node Regsitry and Adds Message to the Message Queue. | ✅ **Complete** |
| **(Server Node) Serial Bridge** | 5 | Forwards Messages in the Message Queue to the UART. | ✅ **Complete** |
| **(Server Node) Node Manager** | 7 | Tracks Nodes Life, sends Alert if a Node dies. | ✅ **Complete** |
| **(Server Node) Scheduling/Threads** | - | RMS Scheduling, Mutex Locks for resources and Threading to run all 3 Services. | ✅ **Complete** |

## 📂 Project Structure

The project follows a modular C structure:

```text
aeris_firmware/
├── app.overlay          # Device Tree (I2C Pin Definitions)
├── CMakeLists.txt       # Build Configuration
├── prj.conf             # Zephyr Kernel Config (Logging, I2C, FPU)
├── sensor_node/src/
│   ├── main.c           # Scheduler & Main Loop
│   └── modules/         # Independent Microservices
│       ├── system_health.c   # (Done) Health Logic & Drift Check
│       ├── system_health.h   # (Done) Public Interface
│       ├── vtt_model.c     # (Done) Main Algo for VTT Model
│       ├── vtt_model.h     # (Done) Public Interface of VTT Model
│       ├── messaging_service.c     # (Done) Main Algo for Messaging Service
│       └── messaging_service.h       # (Done) Public Interface of Messaging Service
├── server_node/src/
│   ├── main.c           # Scheduler & Main Loop
│   └── modules/         # Independent Microservices
│       ├── network_listener.c   # (Done) Listens to the CoAP Network
│       ├── network_listener.h   # (Done) Public Interface
│       ├── serial_bridge.c     # (Done) Forwards the Message Queue data to the UART
│       ├── serial_bridge.h     # (Done) Public Interface of Serial Bridge
│       ├── node_manager.c     # (Done) Updates Node Registry and sends alert if new node joins or dies
│       └── node_manager.h       # (Done) Public Interface of Node Manager
└──
```

Made with ❤️ by muzamil.py