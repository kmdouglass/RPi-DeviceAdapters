IDIR=src
CXX=g++
CXXFLAGS=-std=c++11 -Wall -I$(IDIR)

DEPS=src/gpio.h

MKDIR_P=mkdir -p
DIRECTORIES=bin

#---------------------------------------------------------------------
# Rules
#---------------------------------------------------------------------
.PHONY: all directories

all: directories bin/tests

directories: $(DIRECTORIES)	

$(DIRECTORIES):
	$(MKDIR_P) $@

bin/tests: src/gpio.c tests/tests.cpp
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

clean:
	rm -rf $(DIRECTORIES)
