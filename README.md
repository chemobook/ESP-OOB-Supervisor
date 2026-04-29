<div align="center">

# 🛰️ ESP-OOB-Supervisor

**A low-power maintenance controller for remote LoRa / Meshtastic nodes.**

[![Status: Early Development](https://img.shields.io/badge/Status-Early_Development-orange.svg)](#-status)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Hardware: ESP32](https://img.shields.io/badge/Hardware-ESP32-green.svg)](#-recommended-hardware-architecture)

</div>

The ESP32 supervisor normally stays in **deep sleep**. When it receives a valid **LoRa wake packet**, it wakes up, connects to Wi-Fi (or starts its own Access Point), and opens a local Web UI for service operations.

---

## 🎯 Purpose

Remote LoRa nodes are often installed in hard-to-reach places:
* 🗼 Poles & Masts
* 🏠 Roofs
* 🌳 Trees
* ⛰️ Remote outdoor points

Physical access is difficult, slow, or expensive. **ESP-OOB-Supervisor** allows the administrator to wake, configure, reset, and flash the target node without removing it from its location.

---

## ⚙️ Main Features

| Feature | Description |
|---|---|
| 💤 **Deep Sleep** | ESP32 sleeps most of the time to save power. |
| 📡 **LoRa Wake-Up** | Wakes only after receiving a valid LoRa packet. |
| 🔑 **Wake Key** | Simple protection against random/malicious wake packets. |
| 📶 **Wi-Fi Client** | Connects to the last known Wi-Fi network. |
| 📲 **Access Point** | Creates its own AP if external Wi-Fi is not available. |
| 🌐 **Web UI** | Local maintenance interface for direct management. |
| 🔌 **UART Access** | Serial access to the target node. |
| 🔁 **Hardware Reset** | Direct control over the Reset / EN pin. |
| 🧷 **Bootloader Mode**| BOOT pin control for flashing the target. |
| ⬆️ **Firmware Upload**| Manual firmware upload through the Web UI. |
| ☁️ **Auto Download** | Optional firmware download from an update server. |
| ⏱️ **Auto Sleep** | Automatically returns to sleep after a timeout. |

---

## 🔄 Basic Workflow

```mermaid
flowchart TD
    A([LoRa wake packet received]) --> B[ESP32 wakes up]
    B --> C{Known Wi-Fi nearby?}
    C -- Yes --> D[Connect to Wi-Fi]
    C -- No --> E[Start internal Access Point]
    D --> F[Web UI becomes available]
    E --> F
    F --> G[Admin services the target node]
    G --> H([ESP32 returns to deep sleep])
    
    style A fill:#4CAF50,stroke:#388E3C,color:white
    style H fill:#2196F3,stroke:#1976D2,color:white
