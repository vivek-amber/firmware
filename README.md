# Amber Edge Firmware

Firmware for ESP32-powered IoT devices. The project enables reliable device connectivity, telemetry reporting, remote command execution, device health monitoring, and seamless integration with backend services through MQTT.

---

## Overview

Firmware acts as the device-side component of the Amber IoT platform. It is responsible for:

- Secure Wi-Fi connectivity
- MQTT communication with cloud services
- Device telemetry collection and reporting
- Real-time command processing
- Device health monitoring
- Error detection and reporting
- Persistent configuration storage
- OTA-ready firmware architecture
- Reliable recovery from connectivity failures

---

## Current Status

### Implemented

✅ ESP32 firmware foundation

✅ Wi-Fi provisioning and connectivity

✅ MQTT communication layer

✅ Command request handling

✅ Device state synchronization

✅ Real-time telemetry publishing

✅ Device configuration management

✅ NVS-based persistent storage

✅ Error reporting framework

✅ FreeRTOS task architecture

✅ MQTT reconnect and recovery logic

✅ Protobuf-based device messaging

---

### In Progress

🚧 OTA firmware updates

🚧 Advanced device diagnostics

🚧 Version management system

🚧 Improved fault recovery workflows

🚧 Event-driven state machine implementation

---

### Planned

📌 Secure firmware updates

📌 Rollback support

📌 Firmware progress reporting

📌 Remote configuration updates

📌 Edge analytics

📌 Device activity logs

📌 Fleet-wide update management

📌 Enhanced security and certificate management

---

## Architecture

```text
                   ┌─────────────────┐
                   │ MQTT Broker     │
                   └───────┬─────────┘
                           │
                           ▼
                ┌─────────────────────┐
                │   ESP32 Firmware    │
                └─────────┬───────────┘
                          │
          ┌───────────────┼──────────────┐
          ▼               ▼              ▼
    ┌──────────┐    ┌──────────┐   ┌──────────┐
    │ Telemetry│    │ Commands │   │ Health   │
    │  Service │    │ Handler  │   │ Monitor  │
    └────┬─────┘    └────┬─────┘   └────┬─────┘
         │               │              │
         ▼               ▼              ▼
      Sensors      Device Control      Faults
         │                              │
         └──────────────┬───────────────┘
                        ▼
                 ┌───────────┐
                 │ NVS Store │
                 └───────────┘
```

---

## Features

### Device Connectivity

- Wi-Fi connection management
- Automatic reconnect handling
- Connection state monitoring
- Backend communication through MQTT

### Telemetry Reporting

- Live sensor data publishing
- Device operational metrics
- Health status reporting
- Periodic heartbeat messages

### Command Processing

- Remote command execution
- Parameter validation
- State synchronization
- Command acknowledgement

### Fault Management

- Device error detection
- Fault code generation
- Health event reporting
- Recovery event tracking

### Persistence

- NVS-based device configuration
- Saved network credentials
- Stored device settings
- Last known operational state

---

## Technology Stack

### Firmware

- ESP-IDF
- FreeRTOS
- C/C++
- MQTT Client
- Protocol Buffers (Protobuf)
- NVS Storage

### Communication

- Wi-Fi
- MQTT
- TCP/IP
- Protobuf Messaging

### Development

- ESP-IDF Toolchain
- CMake
- Python Utilities

---

## Prerequisites

Before building the firmware, ensure you have:

- ESP-IDF v5+
- Python 3.10+
- ESP32 development board
- USB/UART interface
- Wi-Fi access

---

## Getting Started

### Clone Repository

```bash
git clone https://github.com/vivek-amber/amber-edge-firmware.git
cd amber-edge-firmware
```

### Configure Target

```bash
idf.py set-target esp32
```

### Configure Project

```bash
idf.py menuconfig
```

### Build Firmware

```bash
idf.py build
```

### Flash Device

```bash
idf.py -p COM3 flash
```

### Monitor Logs

```bash
idf.py monitor
```

---

## Device Capabilities

### Connectivity

- Wi-Fi provisioning
- MQTT connectivity
- Automatic reconnection
- Network recovery

### Telemetry

- Sensor measurements
- Runtime statistics
- Device health status
- State synchronization

### Control

- Remote command execution
- Configuration updates
- Runtime parameter changes

### Reliability

- Watchdog monitoring
- Fault recovery
- Persistent storage
- Connection resilience

---

## Communication Flow

```text
Backend
   │
   ▼
MQTT Broker
   │
   ▼
ESP32 Device
   │
   ├── Receive Commands
   ├── Execute Actions
   ├── Publish Telemetry
   └── Report Errors
```

---

## Future OTA Roadmap

### Phase 1

- Firmware version tracking
- OTA partition configuration
- Update availability checks

### Phase 2

- HTTPS firmware download
- Image validation
- OTA progress reporting

### Phase 3

- Rollback support
- Failure recovery
- Health verification

### Phase 4

- Fleet-wide deployments
- Staged rollouts
- Firmware analytics
- Device targeting

---


## Repository Structure

```text
firmware/
├── main/
├── proto
└── CMakeLists.txt
```

---

## Author

### Vivek

Software Engineer focused on building end-to-end IoT platforms spanning embedded devices, backend microservices, and mobile applications.

### Core Areas

- Embedded Systems
- ESP32 Development
- FreeRTOS
- MQTT Communication
- IoT Platforms
- Spring Boot Microservices
- React Native
- Real-Time Systems
- OTA Infrastructure
- Device Connectivity

---

## Vision

Firmware serves as the intelligent device layer of the Amber IoT ecosystem, enabling secure communication, real-time telemetry, remote control, and scalable device management while ensuring reliability, resilience, and maintainability for connected products.