#!/usr/bin/env bash

set -euo pipefail

cd googletest

# Build googletest
cmake CMakeLists.txt
make

# Install
sudo cp -a googletest/include/gtest /usr/include
sudo cp googlemock/gtest/*.a /usr/lib/
