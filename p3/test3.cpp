#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "bloomfilter.h"

using std::cout;
using std::endl;
using std::string;
using std::chrono::duration_cast;
using HR = std::chrono::high_resolution_clock;
using HRTimer = HR::time_point;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::filesystem::path;

static constexpr uint64_t RANDOM_SEED = 42;
static const uint32_t bucket_count = 1000;
static constexpr uint64_t MAX_OPERATIONS = 1e+15;

/** total number of operations */
uint64_t NUM_OPS = 1e5;
/** percentage of insert queries */
uint64_t INSERT = 80;
/** number of iterations */
uint64_t runs = 2;

unsigned int NUM_THREADS = std::thread::hardware_concurrency(); // Default to hardware concurrency


// Test case 1: Basic functionality test
void test_basic_functionality() {
    BloomFilter bf(1<<24);
    std::cout << "\n=== Running Basic Functionality Test ===\n";

    // Test empty filter
    assert(!bf.contains(123));
    std::cout << "Empty filter test passed.\n";

    // Add single element
    bf.add(123);
    assert(bf.contains(123));
    std::cout << "Single element insertion test passed.\n";

    // Check false positives
    constexpr int NUM_CHECKS = 1000;
    int false_positives = 0;
    for (int i = 0; i < NUM_CHECKS; ++i) {
        if (bf.contains(1000 + i)) {
            false_positives++;
        }
    }
    const double fp_rate = (false_positives * 100.0) / NUM_CHECKS;
    std::cout << "Basic test false positive rate: " << fp_rate << "%\n";
}

// Test case 2: Bulk operations and collision test
void test_bulk_operations() {
    BloomFilter bf(1<<24);
    std::cout << "\n=== Running Bulk Operations Test ===\n";

    constexpr uint32_t NUM_ELEMENTS = 1000000;
    std::vector<uint32_t> elements(NUM_ELEMENTS);
    std::iota(elements.begin(), elements.end(), 1);

    for (const auto& e : elements) {
        bf.add(e);
    }

    // Verify insertions
    for (const auto& e : elements) {
        assert(bf.contains(e));
    }
    std::cout << "All inserted elements found.\n";

    int false_positives = 0;
    constexpr uint32_t RANGE = 20000000;
    for (uint32_t i = NUM_ELEMENTS + 1; i <= RANGE; ++i) {
        if (bf.contains(i)) {
            false_positives++;
        }
    }
    const double fp_rate = (false_positives * 100.0) / (RANGE - NUM_ELEMENTS);
    std::cout << "Bulk test false positive rate: " << fp_rate << "%\n";
}

struct ThreadArgs
{
    BloomFilter *bf;
    const uint32_t *arr;
    uint64_t num_ops;
    int thread_id;
    std::set<uint32_t> *expected_set;
    std::atomic<uint64_t> *total_checks;
    std::atomic<uint64_t> *false_positives;
    std::atomic<uint64_t> *false_negatives;
};

void worker_thread(ThreadArgs args) {
    std::mt19937 gen(RANDOM_SEED + args.thread_id);
    std::uniform_int_distribution<> dis(0, 7);

    for (uint64_t i = 0; i < args.num_ops; ++i) {
        const uint32_t elem = args.arr[i];
        if (dis(gen) == 0) {
            args.bf->add(elem);
        } else {
            const bool actual = args.bf->contains(elem);
            const bool expected = args.expected_set->count(elem);
            
            args.total_checks->fetch_add(1);
            if (actual != expected) {
                if (expected) {
                    args.false_negatives->fetch_add(1);
                } else {
                    args.false_positives->fetch_add(1);
                }
            }
        }
    }
}

void read_data(path pth, uint64_t n, uint32_t *data)
{
    FILE *fptr = fopen(pth.string().c_str(), "rb");
    string fname = pth.string();
    if (!fptr)
    {
        string error_msg = "Unable to open file: " + fname;
        perror(error_msg.c_str());
    }
    int freadStatus = fread(data, sizeof(uint32_t), n, fptr);
    if (freadStatus == 0)
    {
        string error_string = "Unable to read the file " + fname;
        perror(error_string.c_str());
    }
    fclose(fptr);
}

int main(int argc, char *argv[]) {
    test_basic_functionality();
    test_bulk_operations();

    path cwd = std::filesystem::current_path();
    path path_insert_values = cwd / "random_values_insert.bin";
    assert(std::filesystem::exists(path_insert_values));

    // Read data from file
    auto *values_insert = new uint32_t[NUM_OPS];
    read_data(path_insert_values, NUM_OPS, values_insert);

    // Precompute expected elements
    std::set<uint32_t> expected_elements;
    uint64_t offset = 0;
    uint64_t enq_ops = NUM_OPS / NUM_THREADS;
    uint64_t extra_enq = NUM_OPS % NUM_THREADS;

    for (unsigned int t = 0; t < NUM_THREADS; ++t) {

        uint64_t thread_enq = enq_ops + (t < extra_enq ? 1 : 0);
        std::mt19937 gen(RANDOM_SEED + t);
        std::uniform_int_distribution<> dis(0, 7);

        for (uint64_t i = 0; i < thread_enq; ++i) {
            const uint32_t elem = values_insert[offset + i];
            if (dis(gen) == 0) {
                expected_elements.insert(elem);
            }
        }
        offset += thread_enq;
    }

    // Main test loop
    for (uint32_t run = 0; run < runs; ++run) {
        std::atomic<uint64_t> total_checks{0};
        std::atomic<uint64_t> false_positives{0};
        std::atomic<uint64_t> false_negatives{0};

        BloomFilter bf(1 << 24);
        std::vector<std::thread> threads(NUM_THREADS);

        for (unsigned int i = 0; i < NUM_THREADS; ++i)
        {
            uint64_t thread_enq = enq_ops + (i < extra_enq ? 1 : 0);

            ThreadArgs args;
            args.bf = &bf;
            args.arr = values_insert + offset;
            args.num_ops = thread_enq;
            args.thread_id = i;
            args.expected_set = &expected_elements;
            args.false_negatives = &false_negatives;
            args.false_positives = &false_positives;

            threads[i] = std::thread(worker_thread, args);
            offset += thread_enq;
        }

        // Report results
        const uint64_t checks = total_checks.load();
        if (checks > 0) {
            const double fp_rate = (false_positives.load() * 100.0) / checks;
            std::cout << "Run " << run + 1 
                      << " | Elements: " << expected_elements.size()
                      << " | FP Rate: " << fp_rate << "%\n";
        }
        
        if (false_negatives.load() > 0) {
            std::cerr << "Error: " << false_negatives.load() 
                      << " false negatives detected!\n";
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}