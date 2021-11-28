# fReEFLEX GUI
[![License: Apache License 2.0](https://img.shields.io/badge/License-Apache%20License%202.0-yellow.svg)](LICENSE)

> fReEFLEX controller firmware.

[](doc/controller_opt101.png?raw=true "controller")

## 📁 Download
- [releases](https://github.com/fReEFLEX/fReEFLEX-controller/releases)

## Contents
- [Download](#-download)
- [What is this?](#what-is-this)
- [Required hardware](#required-hardware)
- [Installation](#installation)
- [Support the fReEFLEX project](#-support-freeflex-project)
- [License](#-license)

## What is this?
fReEFLEX helps you to 
- Find the best settings for low input latency in 3D Games
- Optimize your OS for low input latency
- Measure processing delay of mice

The fReEFLEX project includes
- [Controller firmware](https://github.com/fReEFLEX/fReEFLEX-controller/releases) - copy this on your Raspberry Pi Pico
- [GUI](https://github.com/fReEFLEX/fReEFLEX-GUI/) - to operate the controller
- [3D Application](https://github.com/fReEFLEX/fReEFLEX-clicker/) - 3D application for E2E latency measurement 


## Required hardware
A fReEFLEX controller can be built in three variants.
### 1. Raspberry Pi Pico
The simplest version of a fReEFLEX controller is a bare [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/). 
- can measure system latency but not E2E (click-to-photon)



## Installation
1. Hold down the **BOOTSEL** button when you plug in your Pico to you.
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


