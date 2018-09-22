# Build

Build tools for compiling Micro-Manager for the Raspberry Pi.

## Description

These resources define the build environment for cross-compiling
Micro-Manager for the Raspberry Pi's ARM architecture on a x86
workstation. The files include

- **Dockerfile** a Docker image for cross-compiling Micro-Manager for
  the ARM architectures
- **setup** a bash script that is run inside the build container and
  launches the build
- **docker-compose.yml** configuration file that handles mounting of
  the source code and dependency directories inside the build
  container

## Requirements

- [Micro-Manager source code](https://github.com/micro-manager/micro-manager)
- [Micro-Manager dependencies](https://micro-manager.org/wiki/Micro-Manager_Source_Code) (also called 3rdpartypublic)
- [Docker](https://docs.docker.com/install/)
- [Docker Compose](https://docs.docker.com/compose/install/)

## Building Micro-Manager

To build Micro-Manager for the Rasbperry Pi, you will first need to
ensure that you have the Micro-Manager source code laid out in the
following structure within the `/opt/rpi-micromanager` directory:

```
/opt/rpi-micromanager
├── 3rdpartypublic
├── micro-manager
└── patches
```

- **3rdpartypublic** the SVN repository containing the Micro-Manager
  build dependencies
- **micro-manager** the Git repository of Micro-manager. Currently the
  head of the `mm2` branch is used and must be checked out
- **patches** any optional patches to apply before the build. These
  must be applied in the `setup` script

After you have the directory structure laid out, you can build
Micro-Manager by running the following command from this directory:

```
docker-compose up
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
$ docker tag kmdouglass/rpi-micromanager:build-YYYYMMDD TARGET_IMAGE
$ docker push TARGET_IMAGE
```

where `YYYYMMDD` is the year-month-day that the container is
definition is created on. Note that naming conventions for
`TARGET_IMAGE` follow the format `USER_NAME/IMAGE_NAME:TAG` where
`USER_NAME` is your Dockerhub username and `:TAG` is optional.
