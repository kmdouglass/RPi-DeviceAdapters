IDIR=src
CC=gcc
CCFLAGS=-Wall -I$(IDIR)
CXX=g++
CXXFLAGS=-std=c++11 -Wall -I$(IDIR)

DEPS=src/gpio.h

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

bin/example: src/gpio.c src/example.c
	$(CC) $(CCFLAGS) $^ -o $@

bin/tests: src/gpio.c tests/tests.cpp
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

test: bin/tests
	$<

clean:
	rm -rf $(DIRECTORIES)
