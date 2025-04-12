CXX = g++
CPPFLAGS =
CXXFLAGS = -O3 -std=c++20

# Default target
all: p1.out p2.out p3.out

# Build problem1
p1.out: ./p1/problem1.cpp ./p1/hash_table.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

# Build problem2
p2.out: ./p2/problem2.cpp ./p2/lockfreequeue.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

# Build problem3
p3.out: ./p3/problem3.cpp ./p3/bloomfilter.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

# Clean target
clean:
	rm -f *.out compile_commands.json