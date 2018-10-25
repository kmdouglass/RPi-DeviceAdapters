#!/usr/bin/env bash
# Merges the RPi-DeviceAdapters source code with Micro-Manager's.
#
# USAGE: ./prebuild.sh SRC_DIR
#
# SRC_DIR is the absolute path to the directory that contains the
# Micro-Manager source code and 3rdpartypublic repository.
#
# Kyle M. Douglass, 2018
# kyle.m.douglass@gmail.com
#

set -euo pipefail

BASE_DIR=$(dirname $0)
SRC_DIR=$1
MM_SRC_DIR=$SRC_DIR/micro-manager

rsync -av "${BASE_DIR}/../src/" "${MM_SRC_DIR}/"
