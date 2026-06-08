# 💧 IoT-Based Water Quality Monitoring System

An autonomous, solar-powered edge-computing system designed to monitor drinking water safety in real-time. This project bridges physical mechanical systems with smart IoT electronics to eliminate the need for manual water sampling.

## ⚙️ Core Architecture
* **Edge-Computed Analytics:** Calculates a dynamic Water Quality Index (WQI) locally on the microcontroller to optimize cloud bandwidth.
* **Custom Hybrid Power:** 6V Solar panel + 11.1V 3S Li-Ion UPS stepped down via an LM2596 Buck Converter for stable 3.3V logic protection.
* **Deep Sleep Protocol:** Maximizes battery lifespan by powering down the Wi-Fi radio and CPU between 30-second sampling intervals.
* **Zero-Latency Alerts:** ThingHTTP integration triggers instant Telegram webhook warnings for high TDS, abnormal pH, or extreme Turbidity.

## 🛠️ Hardware & Sensor Array
* ESP32 Dual-Core Microcontroller
* DS18B20 Temperature Sensor
* Analog pH, TDS, and Turbidity Probes
* 0.96" I2C OLED Local Display
* Custom 11.1V UPS Battery Bank

## 💻 Firmware
The firmware is written in C++ using the Arduino IDE. It handles the I2C/Analog sensor polling, WQI edge calculations, local OLED rendering, and REST API HTTP GET requests to the MathWorks ThingSpeak cloud.

## 📄 Complete Technical Documentation
For the full system block diagrams, circuit schematics, and ThingSpeak telemetry performance data, please review the complete whitepaper:
* [Download the 65-Page Technical Report (PDF)](IoT%20Based%20Water%20Quality%20Monitoring%20System.pdf)

---
**Prepared by:** Sudhan S.  
*B.E. Mechanical Engineering | IoT & Edge Computing Integration* 📫 
Contact: sudhan7535@gmail.com  
🔗 [LinkedIn Profile](https://linkedin.com/in/s-sudhan/)
