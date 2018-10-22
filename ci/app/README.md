# App

A Docker image containing the Micro-Manager Python wrapper and device
adapters for the Raspberry Pi.

## Description

These resources define the application environment for running
Micro-Manager's Python wrapper on the Raspberry Pi. The files include

- **Dockerfile** a Docker image containing the application and device adapters
- **Makefile** a convenience tool for creating new Docker images from
  compiled Micro-Manager artifacts

## Requirements

- [Build](../build) artifacts from cross-compiling Micro-Manager for
  the Rasperry Pi
- [Docker](https://docs.docker.com/install/)
- [Make](https://www.gnu.org/software/make/)

## Creating and running the app

After building the Raspberry Pi device adapters, you should find the
compiled artifacts in the `/opt/rpi-micromanager/build`
directory. Once they are there, you may create a new application image
with the command

```
$ make build
```

(You may need to add `sudo` to the beginning of these commands,
depending on how you installed Docker.) To transfer the resulting
image to the Pi, first retag it and upload it to [Docker
Hub](https://hub.docker.com/).

```
$ docker tag kmdouglass/rpi-micromanager USER_NAME/rpi-micromanager
$ docker push USER_NAME/rpi-micromanager
```

where USER_NAME is your Docker Hub username. Next, pull the image by
running the following command from a terminal on your Pi.

```
$ docker pull USER_NAME/rpi-micromanager
```

To run the Python interpreter that contains the image, the command is

```
$ docker run -it --rm --device=/dev/gpiomem:/dev/gpiomem --group-add 997 USER_NAME/rpi-micromanager
```

Replace 997 with the group ID of the `gpio` group in Raspbian if it
differs on your system; USER_NAME is again your Docker Hub
username. The `--device` and `--group-add` flags give the container
access to the GPIO registers on the Pi; they are not necessary if you
will not use the Pi's GPIO pins.
