# Meteorological Balloon

This is my first project using ESP32. The goal was to design and build a telemetry system for a meteorological balloon.

The system collects environmental and positional data during flight, stores it on an SD card, and periodically sends GPS location via SMS to enable recovery after landing.

---

## Project Description

The system acts as a data logger mounted on a meteorological balloon.

During operation it:
- collects data from multiple sensors
- logs data to an SD card
- sends current GPS position via GSM

---

## Features

- GPS positioning (latitude, longitude, altitude)
- SMS with Google Maps link
- data logging to SD card
- time measurement using RTC and GPS
- temperature, pressure and humidity measurement
- motion and orientation measurement (IMU)
- GPS signal quality (HDOP, number of satellites)
- gas detection (MQ2)

---

## Hardware Components

### Microcontroller
- ESP32

### Communication
- SIM800L (GSM / SMS)
- GPS module (TinyGPS++)

### Sensors
- GY-521 (MPU6050) – accelerometer and gyroscope
- BMP280 – temperature, pressure, calculated altitude
- AHT10 – temperature and humidity
- MQ2 – gas sensor

### RTC
- DS3231

### Storage
- SD card

---

## Program Operation

### Initialization (setup)
- initialization of all sensors and modules
- creation of a new data file (e.g. dane1.txt, dane2.txt)
- writing header to the file

---

### Main Loop

The program runs continuously and performs:

#### Data acquisition
- GPS data (position, speed, course)
- number of satellites and HDOP
- IMU data (acceleration, angular velocity)
- environmental data (temperature, pressure, humidity)
- gas sensor readings
- time from RTC and GPS

#### Data logging
- all data is appended to a text file on the SD card

#### SMS transmission
- sent periodically (e.g. every 15 minutes)
- contains a Google Maps link: https://www.google.com/maps?q=LAT,LON
