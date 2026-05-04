# 🍃 KLIMASense — Mold Prevention System (Firmware)

> **Embedded firmware for the KLIMASense distributed IoT mold prediction system.**
> This repository contains the Zephyr RTOS firmware for all three nRF52840 nodes that form the edge computing backbone of the system.

---

## 🌡️ What It Does

This firmware turns Nordic nRF52840 microcontrollers into a real-time, distributed mold prediction network. Each sensor node independently collects temperature and humidity data, runs the **VTT Mold Growth Model** as a local edge computation, monitors its own hardware health, and transmits results wirelessly over an **OpenThread CoAP mesh network** to a central Server Node — which bridges the data to a Raspberry Pi backend over USB Serial.

The system was engineered to be fully **proactive**: it predicts and warns about mold risk before any visible growth occurs, rather than detecting it after the fact.

---

## 🏗️ System Architecture

```
[Sensor Node 1]  ──CoAP/OpenThread──►┐
                                      ├──► [Server Node]  ──USB Serial──►  [Raspberry Pi 5]
[Sensor Node 2]  ──CoAP/OpenThread──►┘     nRF52840                        Data Logger + Dashboard
 nRF52840 + 2× DHT20                        Producer-Consumer Queue
 Edge VTT computation
```

The dashboard and backend live in the companion repository:
👉 **[KLIMA-Sense-Mold-Prevention-Dashboard](https://github.com/muzammilmalik01/KLIMA-Sense-Mold-Prevention-Dashboard)**

---

## ⚙️ Microservices Architecture

The core engineering challenge of this system is running **multiple independent services concurrently** on a microcontroller while sharing a single I2C bus and an OpenThread CoAP radio — without hardware collisions, data loss, or missed deadlines.

The solution is a **microservices architecture** managed by **Zephyr RTOS Fixed Priority Preemptive Scheduling**, with **dual Mutex locks** protecting shared hardware resources:

- 🔒 `sensors_lock` — protects the I2C bus (shared by both DHT20 sensors)
- 🔒 `coap_lock` — protects the CoAP radio transmit buffer

Each microservice is an independent Zephyr thread with a fixed priority level based on its deadline criticality.

---

### 📡 Sensor Node Microservices

Each sensor node (`sensor_node1/`, `sensor_node2/`) runs three independent microservices:

| Microservice | Source File | Priority | Interval | Responsibility |
|---|---|---|---|---|
| 🔴 **System Health** | `system_health.c` | **1 — Highest** | Every 10 s | Verifies I2C connectivity (SDA/SCL lines) and VCC/GND power supply for both DHT20 sensors. Runs a **Drift Detection algorithm** — if Sensor A and Sensor B diverge by more than 5%, a calibration alert fires. On full sensor failure, **automatically switches to the backup sensor** with zero downtime. |
| 🟡 **Telemetry Data** | `messaging_service.c` | **2** | Every 60 s | Safely collects averaged temperature and humidity readings and transmits raw data payloads to the Server Node via CoAP, without blocking the safety-critical health checks above. |
| 🟢 **VTT Model** | `vtt_model.c` | **3 — Lowest** | Every 15 min | Runs the computationally expensive VTT differential equation as a background task — only consuming CPU cycles when the system is idle. Accumulates the mold index over time using discrete time integration (ΔM/Δt). |

> **Why this priority order matters:** System Health at Priority 1 always preempts everything else — a hardware fault is detected instantly regardless of what the other threads are doing. The VTT Model at Priority 3 only runs in idle gaps, so the heavy math never delays telemetry or fault detection.

**VTT Model configuration** (`vtt_model.c`): The sensor nodes are initialised with `VTT_MAT_SENSITIVE` (Class B — Gypsum/Drywall), applying a growth intensity factor k₁ = 1.0 (worst-case scenario). This parameter can be adjusted per room type.

---

### 🖥️ Server Node Microservices

The Server Node (`server_node/`) solves a different problem: incoming CoAP radio packets arrive in microseconds, but printing to USB Serial is comparatively slow. A naive single-loop approach would drop radio packets while waiting for USB to finish.

The solution is a **Producer-Consumer Queue architecture** using a thread-safe Zephyr message queue (`server_queue`, capacity: 10 messages):

| Microservice | Source File | Priority | Responsibility |
|---|---|---|---|
| 🔴 **Network Listener (Producer)** | `network_listener.c` | **1 — Highest** | Preempts all other tasks to capture every incoming CoAP packet instantly. Parses the JSON payload and pushes it into the message queue. Never blocks on I/O. |
| 🟡 **Serial Bridge (Consumer)** | `serial_bridge.c` | **5** | Steadily drains the queue and prints JSON strings to the Raspberry Pi over USB. Uses a `K_FOREVER` wait — consumes **zero CPU** until a message is available. |
| 🟢 **Node Manager** | `node_manager.c` | **7 — Lowest** | Background heartbeat registry. Continuously monitors all connected sensor nodes. If a node stops reporting for more than 15 seconds, it is immediately flagged as **"Offline"** — preventing the dashboard from displaying stale or fake data. |

> In simulation stress tests (3,600× time acceleration), the Producer-Consumer architecture processed an entire 24-hour data storm in 24 seconds with **0% packet loss** and **zero mutex deadlocks**, meeting strict 1000 ms execution deadlines throughout.

---

## 📁 Repository Structure

```
Mold-Prevention-System/
│
├── 📂 sensor_node1/
│   ├── 📂 src/
│   │   ├── 🔧 main.c                         # RTOS thread registration and startup
│   │   └── 📂 modules/
│   │       ├── 🔴 system_health.c/.h          # System Health Microservice (Priority 1)
│   │       ├── 🟡 messaging_service.c/.h      # Telemetry Data Microservice (Priority 2)
│   │       └── 🟢 vtt_model.c/.h              # VTT Model Microservice (Priority 3)
│   ├── 📂 boards/                             # Board-specific DTS overlays
│   ├── 📂 drivers/                            # Custom DHT20 driver
│   ├── 📂 dts/                                # Device tree sources
│   ├── 📄 CMakeLists.txt
│   ├── 📄 Kconfig
│   └── 📄 prj.conf                            # Zephyr kernel + OpenThread config
│
├── 📂 sensor_node2/                           # Identical structure to sensor_node1
│
├── 📂 server_node/
│   ├── 📂 src/
│   │   ├── 🔧 main.c                         # RTOS thread registration and startup
│   │   └── 📂 modules/
│   │       ├── 🔴 network_listener.c/.h       # Network Listener Microservice (Priority 1)
│   │       ├── 🟡 serial_bridge.c/.h          # Serial Bridge Microservice (Priority 5)
│   │       ├── 🟢 node_manager.c/.h           # Node Manager Microservice (Priority 7)
│   │       └── 📄 shared_types.h              # Shared data structures (sensor_data_t)
│   ├── 📂 boards/
│   ├── 📄 CMakeLists.txt
│   └── 📄 prj.conf
│
└── 🚫 .gitignore
```

---

## 🛠️ Tech Stack

| Component | Technology |
|---|---|
| **Microcontrollers** | Nordic nRF52840 (×3 — 2 sensor nodes, 1 server node) |
| **Environmental Sensors** | DHT20 (×2 per sensor node — dual redundancy) |
| **Real-Time OS** | Zephyr RTOS with Fixed Priority Preemptive Scheduling |
| **Mesh Network** | OpenThread (IPv6) |
| **Communication Protocol** | CoAP (Constrained Application Protocol) |
| **Build System** | CMake + Zephyr West |
| **Language** | C |

---

## 🔨 Build & Flash

### Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) installed
- `west` build tool installed
- Nordic nRF52840 DK or Dongle hardware

### Build a Node

```bash
# From the repo root — build sensor_node1
west build -b nrf52840dk/nrf52840 sensor_node1

# Build sensor_node2
west build -b nrf52840dk/nrf52840 sensor_node2

# Build server_node
west build -b nrf52840dk/nrf52840 server_node
```

### Flash

```bash
west flash
```

> Flash each node separately. Ensure the correct board target matches your hardware variant.

---

## 🧪 Simulation Mode

To validate the VTT Model and RTOS scheduling without waiting for weeks of real mold accumulation, the firmware includes a **Simulation Mode** using time acceleration.

- ⚡ **1 real-time second = 1 simulated hour**
- A full 24-hour environmental cycle completes in exactly **24 seconds**
- Telemetry and VTT Model thread sleep periods are reduced from 60 s / 15 min down to **1000 ms** each

This was used to validate two scenarios:

- 🚿 **Scenario A (Bathroom):** Constant 24.0°C / 90% RH for 24 h — confirmed continuous mold index accumulation and crossing of the M=1 Active Growth threshold
- 🛏️ **Scenario B (Bedroom):** Cold/damp night (15°C / 85%) followed by ventilated morning (20°C / 45%) — confirmed proactive Condensation Risk warning and correct negative growth rate (mold index decline)

---

## 📄 Project Status

This repository contains the final prototype firmware developed as part of an academic IoT project at Frankfurt University of Applied Sciences. The hardware setup is no longer actively deployed. Known limitations and unresolved issues are documented in the issue history.
