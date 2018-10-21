IDIR=src/DeviceAdapters/RPiGPIO
CC=gcc
CCFLAGS=-Wall -I$(IDIR)
CXX=g++
CXXFLAGS=-std=c++11 -Wall -I$(IDIR)

DEPS=$(IDIR)/gpio.h

MKDIR_P=mkdir -p
DIRECTORIES=bin

#---------------------------------------------------------------------
# Rules
#---------------------------------------------------------------------
.PHONY: all directories

all: directories bin/tests bin/example

directories: $(DIRECTORIES)	

$(DIRECTORIES):
	$(MKDIR_P) $@

bin/example: src/DeviceAdapters/RPiGPIO/gpio.c src/DeviceAdapters/RPiGPIO/example.c
	$(CC) $(CCFLAGS) $^ -o $@

bin/tests: src/DeviceAdapters/RPiGPIO/gpio.c src/DeviceAdapters/RPiGPIO/tests/tests.cpp
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

test: bin/tests
	$<

clean:
	rm -rf $(DIRECTORIES)
