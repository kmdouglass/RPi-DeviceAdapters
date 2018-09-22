FROM resin/raspberrypi3-debian:stretch
MAINTAINER Kyle M. Douglass <kyle.m.douglass@gmail.com>

RUN [ "cross-build-start" ]

# Get the build dependencies
RUN apt-get update && apt-get -y install --no-install-recommends \
    autoconf \
    automake \
    build-essential \
    git \
    libatlas-base-dev \
    libboost-dev \
    libboost-all-dev \
    libtool \
    patch \
    pkg-config \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    swig \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && pip3 install numpy

RUN [ "cross-build-end" ]

# Set up the mount point for the source files and setup script
ADD setup /micro-manager/
VOLUME /micro-manager/src

WORKDIR /micro-manager/src
ENTRYPOINT [ "/sbin/tini", "-s", "--" ]
CMD [ "/micro-manager/setup" ]