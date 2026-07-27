# 🤖 Autonomous ESP32 Sumo Robot

![C++](https://img.shields.io/badge/Language-C++-blue.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32-lightgrey.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino_IDE-00979D.svg)

## 📌 Project Overview
This repository contains the firmware and architectural documentation for an Autonomous Sumo Robot. The project is designed with a focus on **split-second reaction times**, **hardware stability**, and **robust power management**. 

The core logic is driven by an ESP32 microcontroller, utilizing a non-blocking Finite State Machine (FSM) to handle search, attack, and edge-escape routines dynamically.

## ⚡ Hardware Architecture & Power Management
To prevent voltage drops during high-current motor draws, the system utilizes a **Dual-Battery Isolation Strategy**:
- **Control Circuit:** A 7.4V battery powers an Arduino board, which is strictly repurposed as a stable 5V/3.3V Power Distribution Module to supply the ESP32 and tracking sensors safely.
- **Power Circuit:** A separate 7.4V battery is dedicated entirely to the motor driver and actuators.
- **Signal Protection:** A **Logic Level Converter** safely steps down the 5V Echo signal from the Ultrasonic sensor to the 3.3V logic of the ESP32 to prevent hardware damage.

### 🛠️ Components Used
* **Main Controller:** ESP32 (Core V3.x compatible)
* **Power Distribution:** Arduino (used as a regulator module)
* **Actuators:** 2x DC Motors + Motor Driver (e.g., L298N)
* **Sensors:** 
  * 1x Ultrasonic Sensor (Opponent detection)
  * 4x IR Tracking Sensors (Ring boundary detection)
* **Misc:** Logic Level Converter, 2x 7.4V Batteries.

## 🧠 Software Architecture (Firmware)
The firmware is written in Embedded C++ and avoids blocking functions (like `delay()`) to ensure real-time responsiveness.

* **Finite State Machine (FSM):** Manages the robot's logic through defined states (`INIT_DELAY`, `SEARCHING`, `ATTACKING`, `EDGE_ESCAPE`).
* **Non-Blocking Logic:** Built entirely on `millis()` for time-tracking and simultaneous task execution.
* **Hysteresis & Debouncing:** Custom distance and time thresholds prevent false sensor readings (noise) from interrupting the robot's attack momentum.
* **Absolute Edge Priority:** IR boundary detection overrides all active states instantaneously to prevent ring-outs.
* **Modern ESP32 PWM:** Utilizes the updated `ledcAttach()` and `ledcWrite()` API for precise motor control.

## 🚀 Installation & Setup
1. Clone the repository:
   ```bash
   git clone [https://github.com/YourUsername/ESP32-Sumo-Robot.git](https://github.com/YourUsername/ESP32-Sumo-Robot.git)
