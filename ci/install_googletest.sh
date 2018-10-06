#!/usr/bin/env bash
#
# Kyle M. Douglass, 2018
# kyle.m.douglass@gmail.com
#

set -euo pipefail

cd googletest

# Build googletest
cmake CMakeLists.txt
make

# Install
sudo cp -a googletest/include/gtest /usr/include
sudo cp googlemock/gtest/*.a /usr/lib/
