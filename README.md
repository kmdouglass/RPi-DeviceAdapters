# RPi-DeviceAdapters

[![Build Status](https://travis-ci.org/kmdouglass/RPi-DeviceAdapters.svg?branch=master)](https://travis-ci.org/kmdouglass/RPi-DeviceAdapters)
[![Documentation Status](https://readthedocs.org/projects/rpi-deviceadapters/badge/?version=latest)](https://rpi-deviceadapters.readthedocs.io/en/latest/?badge=latest)

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


## Quickstart

On a Raspberry Pi that has [Docker](https://www.docker.com/) already
installed , create a file named `script.py` that contains the
following lines:

```
import MMCorePy
mmc = MMCorePy.CMMCore()
print(mmc.getVersionInfo())
```

Run the following commands from a terminal window from the same Pi:

```
$ pip3 install tacpho.adapters
$ mm.py pull
$ mm.py run script.py
```

`mm.py pull` may take several minutes to download the
application on the first use.

## Documentation

https://rpi-deviceadapters.readthedocs.io/en/latest/

## Related pages

- **[rpi-micromanager](https://hub.docker.com/r/kmdouglass/rpi-micromanager/)** The project's Docker images
- **[Micro-Manager](https://micro-manager.org/)** Open source microscopy software
