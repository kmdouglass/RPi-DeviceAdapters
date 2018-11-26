# Build

Build tools for compiling Micro-Manager for the Raspberry Pi.

## Description

These resources define the build environment for cross-compiling
Micro-Manager for the Raspberry Pi's ARM architecture on a x86
workstation. The files include

- **Dockerfile** a Docker image for cross-compiling Micro-Manager for
  the ARM architecture
- **setup** a bash script that is run inside the build container and
  launches the build
- **docker-compose.yml** configuration file that handles mounting of
  the source code and dependency directories inside the build
  container
- **ccache.conf** configuration file for the compiler cache. Changes
  to this file will rarely be necessary

## Requirements

- [Micro-Manager source code](https://github.com/micro-manager/micro-manager)
- [Micro-Manager dependencies](https://micro-manager.org/wiki/Micro-Manager_Source_Code) (also called 3rdpartypublic)
- [Docker](https://docs.docker.com/install/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [QEMU](https://www.qemu.org/)

The Micro-Manager source code and dependencies require the following
tools to obtain:

- [Git](https://git-scm.com/)
- [Subversion](https://subversion.apache.org/)

### Setup the QEMU emulator

This project will produce libraries that can only be used on an ARM-based system like the
Pi. Building these libraries on a x86_64 system requires setting up a platform emulator like
QEMU. This may be done as follows:

1. On Ubuntu, install the emulation packages with the commands:

```
$ sudo apt update
$ sudo install qemu qemu-user-static qemu-user binfmt-support
```

2. Register QEMU in the build agent:

```
$ docker run --rm --privileged multiarch/qemu-user-static:register --reset
```

## Building Micro-Manager

To build Micro-Manager for the Rasbperry Pi, you will first need to
ensure that you have the Micro-Manager source code laid out in the
following structure within the `/opt/rpi-micromanager` directory:

```
/opt/rpi-micromanager
├── 3rdpartypublic
└── micro-manager
```

- **3rdpartypublic** the SVN repository containing the Micro-Manager
  build dependencies
- **micro-manager** the Git repository of Micro-manager. Currently the
  head of the `mm2` branch is used and must be checked out

In addition to this directory layout, you will need to create a folder
named `.ccache` (note the double 'c'!) in your `$HOME` directory for
the compiler cache.

There is a convenience script called `prebuild.sh` in the `ci` folder
that may be used to automatically checkout the Micro-Manager source
code and dependencies, creating all the necessary folders in the
process. It will also merge this project's code into the Micro-Manager
source code directory. To use it, pass the installation directory on
the command line when calling the script:

```
$ ci/prebuild.sh /opt/rpi-micromanager
```

After you have the directory structure laid out, you can build
Micro-Manager by running the following commands from this directory:

```
$ docker-compose pull
$ docker-compose up
```

If successful, the build artifacts will be installed in
`/opt/rpi-micromanager/build`.

## Creating the build image

Creating a new build image is not necessary unless you need to modify
the image from which build containers are created. You may wish to do
this, for example, when you need to add a package to the build
container. The `rpi-micromanager:build` image is created out-of-band
using the following commands:

```
$ docker-compose build
```

You can push the resulting image to your own Dockerhub registry by
first retagging it.

```
$ docker tag kmdouglass/rpi-micromanager:build TARGET_IMAGE
$ docker push TARGET_IMAGE
```

Note that naming conventions for `TARGET_IMAGE` follow the format
`USER_NAME/IMAGE_NAME:TAG` where `USER_NAME` is your Dockerhub
username and `:TAG` is optional.

After pushing the new image, you will need to change the
`docker-compose.yml` file to point towards your own image.
