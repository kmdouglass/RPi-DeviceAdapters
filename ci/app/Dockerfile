FROM resin/raspberrypi3-debian:stretch
MAINTAINER Kyle M. Douglass <kyle.m.douglass@gmail.com>

RUN [ "cross-build-start" ]

# Install the run-time dependencies
RUN apt-get update && apt-get -y install --no-install-recommends \
    libatlas-base-dev \
    libboost-all-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && pip3 install numpy

# Copy in the Micro-Manager source files
RUN useradd -ms /bin/bash micro-manager && \
    mkdir -p /home/micro-manager/app/userdata && \
    chown -R micro-manager:micro-manager /home/micro-manager/app
COPY --chown=micro-manager:micro-manager build/. /home/micro-manager/app

RUN [ "cross-build-end" ]

# Final environment configuration
USER micro-manager:micro-manager
VOLUME /home/micro-manager/app/userdata
WORKDIR /home/micro-manager/app/userdata
ENV PYTHONPATH /home/micro-manager/app/lib/micro-manager

ENTRYPOINT ["/sbin/tini", "-s", "--", "/usr/bin/python3"]
