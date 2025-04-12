CXX = g++
CXXFLAGS = -O3
LDFLAGS = -pthread
LDLIBS =

# --- Program 1 Configuration ---
PROG1 = p1
SRCS1 = problem1.cpp hash_table.cpp
OBJS1 = $(SRCS1:.cpp=.o)
$(OBJS1): CXXFLAGS += -std=c++17

# --- Program 2 Configuration ---
PROG2 = p2
SRCS2 = problem2.cpp lockfreequeue.cpp
OBJS2 = $(SRCS2:.cpp=.o)
$(OBJS2): CXXFLAGS += -std=c++17

# --- Program 3 Configuration ---
PROG3 = p3
SRCS3 = problem3.cpp bloomfilter.cpp
OBJS3 = $(SRCS3:.cpp=.o)
$(OBJS3): CXXFLAGS += -std=c++20

# --- Targets ---

# Default target: Build all programs
.PHONY: all
all: $(PROG1) $(PROG2) $(PROG3)

# Linking rule for Program 1
$(PROG1): $(OBJS1)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Linking rule for Program 2
$(PROG2): $(OBJS2)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Linking rule for Program 3
$(PROG3): $(OBJS3)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

ALL_OBJS = $(OBJS1) $(OBJS2) $(OBJS3)
DEPS = $(ALL_OBJS:.o=.d)
-include $(DEPS)

# --- Cleanup ---

.PHONY: clean
clean:
	rm -f $(PROG1) $(PROG2) $(PROG3) $(ALL_OBJS) $(DEPS)
	@echo "Cleaned up build files."