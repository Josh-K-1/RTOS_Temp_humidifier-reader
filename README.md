# Real-Time Temperature Monitor — ESP32 / FreeRTOS

A multi-task FreeRTOS application running on ESP32 that monitors temperature 
and humidity using a DHT11 sensor and drives LED indicators based on 
configurable thresholds.

## Overview

Built to demonstrate concurrent task design, inter-task communication, and 
hardware fault handling in a real-time embedded environment. The system runs 
three independent FreeRTOS tasks at different priorities, communicating 
through a queue rather than shared globals.

## System Architecture

| Task | Priority | Rate | Responsibility |
|---|---|---|---|
| taskReadSensor | 2 (High) | Every 2s | Reads DHT11, writes to queue |
| taskControlLEDs | 1 (Normal) | Every 500ms | Reads queue, drives LED output |
| taskSerialMonitor | 1 (Normal) | Every 5s | Peeks queue, prints status |

## Key Design Decisions

**Queue over global variables** — sensor data is passed between tasks using 
`xQueueOverwrite` and `xQueueReceive` to prevent race conditions and ensure 
only the latest reading is acted upon.

**Non-blocking delays** — `vTaskDelay` yields the CPU during waits, allowing 
lower priority tasks to run while the sensor task sleeps between reads.

**Fault detection** — invalid sensor readings set a `valid` flag in the 
SensorReading struct, triggering a distinct error state (both LEDs blink) 
rather than acting on bad data.

## Hardware

| Component | Pin |
|---|---|
| DHT11 Data | GPIO 21 |
| Green LED | GPIO 16 |
| Red LED | GPIO 17 |
| DHT11 VCC | 3.3V |
| DHT11 GND | GND |

**Note:** DHT11 requires a 10kΩ pull-up resistor between DATA and 3.3V.

## LED Behavior

| State | Green | Red |
|---|---|---|
| Temperature OK (< 77°F) | ON | OFF |
| Temperature too hot (> 77°F) | OFF | ON |
| Sensor read failed | BLINK | BLINK |
| Sensor disconnected | OFF | SLOW FLASH |

## Tools & Environment

- Platform: PlatformIO
- Framework: Arduino (ESP32)
- Language: C++
- RTOS: FreeRTOS (bundled with ESP32 Arduino core)
- Library: Adafruit DHT Sensor Library

## What This Demonstrates

- FreeRTOS task creation and priority-based preemptive scheduling
- Inter-task communication via queues and synchronization primitives
- Hardware fault detection and graceful failure handling
- Embedded C++ on a resource-constrained microcontroller
- Hardware debugging — resolved sensor wiring issue and missing pull-up 
  resistor using systematic diagnosis
