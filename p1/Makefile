# Compiler settings
# CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 # TBB needs C++11
PTHREAD_FLAGS = -pthread

# Source files
PTHREAD_SRC = hash_table.cpp
DRIVER_SRC = main.cpp
HEADER = hash_table.h

# Target executables
PTHREAD_TARGET = ht_pthread_test
TBB_TARGET = ht_tbb_test

# Default target
all: $(PTHREAD_TARGET)

# build Pthread version
$(PTHREAD_TARGET): $(DRIVER_SRC) $(PTHREAD_SRC) $(HEADER)
	$(CXX) $(CXXFLAGS) $(PTHREAD_FLAGS) $(DRIVER_SRC) $(PTHREAD_SRC) -o $(PTHREAD_TARGET)

clean:
	rm -f $(PTHREAD_TARGET) *.o core.*