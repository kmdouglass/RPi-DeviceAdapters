# RPi-DeviceAdapters

[![Build Status](https://travis-ci.org/kmdouglass/RPi-DeviceAdapters.svg?branch=master)](https://travis-ci.org/kmdouglass/RPi-DeviceAdapters)

RPi-DeviceAdapters is a tool for controlling microscope hardware with
the [Raspberry Pi](https://www.raspberrypi.org/). It provides a common
software interface to different types of peripherals through
[Micro-Manager](https://micro-manager.org/), an open source software
package widely used in microscopy.

RPi-DeviceAdapters contains the following tools:

- **[DeviceAdapters](src/DeviceAdapters)** Ready-made
  Micro-Manager device adapters for various hardware peripherals
- **[build](ci/build)** A Docker image for cross-compiling
  Micro-Manager for the Raspberry Pi's ARM processors on a x86 laptop
  or workstation
- **[app](ci/app)** A Docker image for running the Micro-Manager
  Python wrapper on the Raspberry Pi and that can be easily downloaded
  onto any Pi that has Docker installed on it
