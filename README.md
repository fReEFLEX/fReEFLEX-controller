# fReEFLEX Controller
[![License: Apache License 2.0](https://img.shields.io/badge/License-Apache%20License%202.0-yellow.svg)](LICENSE)

> fReEFLEX controller firmware.

![](doc/breadboard.png?raw=true "Breadboard")

## 📁 Download
- [releases](https://github.com/fReEFLEX/fReEFLEX-controller/releases)

## Contents
- [Download](#-download)
- [What is this?](#what-is-freeflex)
- [Required hardware](#required-hardware)
- [Build it](#build-it)
- [Installation](#firmware-installation)
- [Support fReEFLEX project](#-support-freeflex-project)
- [License](#-license)

## What is fReEFLEX?
fReEFLEX helps you to 
- Find the best settings for low input latency in 3D Games
- Optimize your OS for low input latency
- Measure processing delay of mice
- Measure the frequency of pulsed light sources, e.g. backlight strobing, dimmed LEDs.

The fReEFLEX project includes
- [Controller firmware](https://github.com/fReEFLEX/fReEFLEX-controller/releases) - copy this on your Raspberry Pi Pico
- [GUI](https://github.com/fReEFLEX/fReEFLEX-GUI/) - to operate the controller
- [3D Application](https://github.com/fReEFLEX/fReEFLEX-clicker/) - 3D application for E2E latency measurement 

## Required hardware
>If you don't want to solder anything make sure to get a Pico with **pre soldered headers**, a **Breadboard** and some **Jumper Wires**.

A fReEFLEX controller can be built in three steps, each extending its capabilities:
1. [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) - Measure [System Latency](https://github.com/fReEFLEX/fReEFLEX-GUI/#5-system-latency-mode)
2. OPT101 light sensor (often as CJMCU-101) - Measure [E2E Latency](https://github.com/fReEFLEX/fReEFLEX-GUI/#4-e2e-latency-mode) and [Light frequency](https://github.com/fReEFLEX/fReEFLEX-GUI/#6-light-frequency)
3. a spare mouse - Measure [Mouse latency](https://github.com/fReEFLEX/fReEFLEX-GUI/#51-mouse-latency)

>Measuring with a real mouse is by far the most advanced step but only needed if you want to measure the latency of a specific mouse.
The mouse needs to have the left mouse button pulled to ground when pushed. Otherwise, it currently is not compatible with a fReEFLEX Controller.

Mice that are known work:
- Logitech G203


## Build it
⚠️If you are building the Controller on a Breadboard check above image. 

![](doc/schematic.png?raw=true "Breadboard")

## Firmware installation
1. Hold down the **BOOTSEL** button when you plug in your Pico to your PC.
    >This puts your Pico into Bootsel mode and it appears as a mass storage device on your PC.
2. Copy the [latest firmare](https://github.com/fReEFLEX/fReEFLEX-controller/releases) (.uf2) onto your Pico.
3. The Pico automatically reboots and is ready to be used with the [GUI](https://freeflex.github.io/fReEFLEX-GUI/)

## ☕ Support fReEFLEX project

[PayPal](https://paypal.me/Pmant)

## 👤 Author

[@Pmant](https://github.com/Pmant)

## 📝 License

Copyright © 2021 [Pmant](https://github.com/Pmant).

This project is [Apache License 2.0](LICENSE) licensed.


