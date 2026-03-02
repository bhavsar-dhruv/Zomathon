# 🍽️ Zomato Smart Kitchen Kit: IoT-Powered KPT Optimization

[![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue.svg)](https://www.espressif.com/)
[![Python](https://img.shields.io/badge/Simulation-Python-yellow.svg)](https://www.python.org/)
[![Status](https://img.shields.io/badge/Status-Hackathon_Submission-success.svg)]()

Redefining Kitchen Preparation Time (KPT) prediction accuracy for Zomato by moving beyond subjective manual inputs to capture physical ground truth via zero-friction edge IoT.

## 🚀 The Core Challenge

Zomato’s current KPT prediction model relies heavily on the "Food Ready" (FOR) signal marked manually by merchants on a tablet. This creates two massive data blindspots:
1. **The "Live Rush" Blindspot:** The system is completely blind to the hidden workload of offline dine-in customers and competitor orders.
2. **The Merchant FOR Bias:** Manual signals are noisy. Panicked merchants frequently press "Ready" early to summon riders, poisoning the ML model's training data.

**The Result:** Garbage In, Garbage Out. Riders are dispatched early, leading to high wait times and severe P90 ETA prediction errors.

## 💡 Our Solution: Frictionless Edge IoT

To fix the downstream ML model, we fundamentally upgraded the upstream data pipeline. The **Zomato Smart Kitchen Kit** is a two-module, low-cost hardware suite deployed to the top 10% of high-volume kitchens to capture unbiased ground-truth data.

### 1. Peel-and-Stick Thermal Rush Sensor (Prep Zone)
An ultra-low-power, wire-free edge node that calculates a real-time **Kitchen Rush Index**.
* **Hardware:** ESP32-C3, Panasonic AMG8833 Grid-EYE (8x8 thermal array), AM312 PIR Motion Sensor.
* **Function:** Uses local frame-differencing math to track kinetic kitchen chaos without violating privacy.
* **Architecture:** Sleeps 99% of the time. Wakes via PIR interrupt, captures a 64-pixel thermal frame, and broadcasts the payload in <150ms using ESP-NOW. Runs for 12 months on 3 standard AA batteries.

### 2. The Smart Cubby Station (Packing Zone)
A modular, zero-click weight and thermal verification system that establishes an **Automated Physical Timestamp**.
* **Hardware:** ESP32-WROOM Master Node, multiplexed HX711 Load Cells, DS18B20 1-Wire Digital Temperature Sensors.
* **Function:** Bypasses the merchant tablet entirely. When a bagged order is placed in an assigned cubby, the load cell and thermal sensor instantly verify the digital cart profile, triggering an unfakeable, zero-click FOR timestamp.
* **Architecture:** Magnetic pogo-pin routing allows kitchens to seamlessly snap on additional pods to scale capacity dynamically.

## 📊 Impact & ML Integration

This hardware acts as a pristine data acquisition engine. By enriching Zomato's baseline KPT model with the new `Rush Index` and replacing biased merchant timestamps with automated `AFR (Automated Food Ready)` signals, our simulation yielded massive improvements:

* **74% Reduction in P90 Prediction Errors** (Dropped from 14.25 minutes to 3.63 minutes).
* **Significant drop in RMSE** during unexpected "Friday Night" offline traffic spikes.
* **The Halo Effect:** Training the global ML model on ground-truth data from the top 10% of equipped kitchens universally improves KPT accuracy for the remaining 90% of unequipped restaurants.


