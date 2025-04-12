CXX = g++
CPPFLAGS =
CXXFLAGS = -O3 -std=c++20
LDFLAGS =

# --- Source Files ---
P1_SOURCES_CUSTOM = ./p1/problem1.cpp ./p1/hash_table.cpp
P1_SOURCES_TBB = ./p1/problem1.cpp

P2_SOURCES_CUSTOM = ./p2/problem2.cpp ./p2/lockfreequeue.cpp
P2_SOURCES_BOOST = ./p2/problem2.cpp

P3_SOURCES = ./p3/problem3.cpp ./p3/bloomfilter.cpp
P3_TEST_SOURCES = ./p3/test3.cpp ./p3/bloomfilter.cpp


# Problem 1 - TBB specific flags
P1_CPPFLAGS_TBB = -DUSE_TBB
P1_LDFLAGS_TBB = -ltbb

PTHREAD_LDFLAG = -lpthread

# Problem 2 - Boost specific flags
P2_CPPFLAGS_BOOST = -I/usr/include -DUSE_BOOST_QUEUE
P2_LDFLAGS_BOOST = -lboost_atomic $(PTHREAD_LDFLAG)

# Problem 2 - Common flags
P2_CXXFLAGS_COMMON = -march=native

# Problem 3 specific flags
P3_CXXFLAGS =
P3_LDFLAGS = $(PTHREAD_LDFLAG)

# --- Build Rules ---

# Default target builds the standard/custom versions
all: p1.out p2.out p3.out p1_tbb.out p2_boost.out p3_test.out

# Build problem 1 (Custom HashTable Version)
p1.out: $(P1_SOURCES_CUSTOM)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(PTHREAD_LDFLAG)

# Build problem 1 (TBB concurrent_hash_map Version)
p1_tbb.out: $(P1_SOURCES_TBB)
	$(CXX) $(CPPFLAGS) $(P1_CPPFLAGS_TBB) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(P1_LDFLAGS_TBB) $(PTHREAD_LDFLAG)

# Build problem 2 (Custom LockFreeQueue)
p2.out: $(P2_SOURCES_CUSTOM)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(P2_CXXFLAGS_COMMON) $^ -o $@ $(LDFLAGS) $(PTHREAD_LDFLAG)

# Build problem 2 (Boost LockFreeQueue)
p2_boost.out: $(P2_SOURCES_BOOST)
	$(CXX) $(CPPFLAGS) $(P2_CPPFLAGS_BOOST) $(CXXFLAGS) $(P2_CXXFLAGS_COMMON) $^ -o $@ $(LDFLAGS) $(P2_LDFLAGS_BOOST)

# Build problem 3
p3.out: $(P3_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(P3_CXXFLAGS) $^ -o $@ $(LDFLAGS) $(P3_LDFLAGS)

# Build problem 3
p3_test.out: $(P3_TEST_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(P3_CXXFLAGS) $^ -o $@ $(LDFLAGS) $(P3_LDFLAGS)

# Target to explicitly build the TBB version of P1
build_p1_tbb: p1_tbb.out

# Target to explicitly build the Boost version of P2
build_p2_boost: p2_boost.out

clean:
	rm -f p1.out p1_tbb.out p2.out p2_boost.out p3.out p3_test.out *.o

.PHONY: all clean build_p1_tbb build_p2_boost