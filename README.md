# Meshtastic-OOB-Supervisor

An ESP32-based hardware supervisor for Out-of-Band Management (OOBM) of autonomous Meshtastic nodes and routers via a Telegram bot and a transparent TCP proxy.

## ⚠️ The Problem
Remote autonomous devices (like solar-powered LoRa nodes on trees or hidden routers) occasionally freeze, require hard resets, or need firmware updates. 
* Relying on the target device's internal Wi-Fi is power-hungry and risky (high chance of bricking during Over-The-Air updates).
* Exposing management ports to the public internet invites scanners and botnets.
* Physical access to the device is often difficult or impossible.

## 💡 The Solution
A hardware-independent "watchdog" and transparent gateway built on a low-cost ESP32 microcontroller. It connects to the target device (e.g., Heltec, Wio, OpenWRT Router) via physical wires (UART / GPIO) and acts as an isolated bridge between the hardware and your Telegram bot.

## ✨ Key Features
* **Zero-Trust Security:** No open ports on the internet. The server generates a dynamic, single-use TCP port with a 60-second window strictly upon request from an authorized Telegram user.
* **Transparent TCP Proxy:** The ESP32 blindly forwards bytes between a TCP socket and the target device's serial port. Manage remote nodes using the **official Meshtastic mobile app** or connect via SSH to a router, exactly as if connected via USB.
* **Hardware Watchdog:** Remotely hard-reset a frozen device by triggering its `RST` or `Power` pins via a Telegram command.
* **Captive Portal Setup:** Easy initial deployment. Enter your Wi-Fi credentials, Telegram ID, and server URL through a web interface on the first boot.
* **Deep Sleep & Solar-Friendly:** Optimized for off-grid setups. The ESP32 sleeps most of the time, waking up only on a schedule to send telemetry or listen for lightweight WebSocket commands.
* **Self-Hosted Backend:** The routing server and Telegram bot are easily deployable on any VPS using Docker.

## 🛠️ Repository Structure
* `/firmware` — C++ source code for the ESP32 (PlatformIO project). Handles Wi-Fi, Captive Portal, WebSockets, and UART bridging.
* `/server` — Lightweight asynchronous Python backend. Manages Telegram bot interactions, dynamic port routing, and the SQLite user database.
* `docker-compose.yml` — One-click deployment for the server side.

## 🛒 Hardware Requirements (BOM)
1. **ESP32 Microcontroller:** ESP32-C3 Super Mini is highly recommended for its low power consumption.
2. **Target Device:** Heltec v4, Seeed XIAO, OpenWRT Router, Raspberry Pi, etc.
3. **Wiring:** 4 standard jumper wires (3.3V, GND, TX, RX). Optional 5th wire for hardware reset.

## 🚀 Getting Started
*(Instructions for flashing the ESP32 and deploying the Docker container will be added soon).*

## 📄 License
This project is licensed under the MIT License.
