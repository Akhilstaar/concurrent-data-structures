#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <pthread.h>
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

typedef struct
{
    uint32_t key;
    uint32_t value;
} KeyValue;

// Pack key-value into a 64-bit integer
inline uint64_t packKeyValue(uint32_t key, uint32_t val)
{
    return (static_cast<uint64_t>(key) << 32) |
           (static_cast<uint32_t>(val) & 0xFFFFFFFF);
}

// Function to unpack a 64-bit integer into two 32-bit integers
inline void unpackKeyValue(uint64_t value, uint32_t &key, uint32_t &val)
{
    key = static_cast<uint32_t>(value >> 32);
    val = static_cast<uint32_t>(value & 0xFFFFFFFF);
}

void create_file(path pth, const uint32_t *data, uint64_t size)
{
    FILE *fptr = NULL;
    fptr = fopen(pth.string().c_str(), "wb+");
    fwrite(data, sizeof(uint32_t), size, fptr);
    fclose(fptr);
}

/** Read n integer data from file given by pth and fill in the output variable
    data */
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

// These variables may get overwritten after parsing the CLI arguments
/** total number of operations */
uint64_t NUM_OPS = 1e8;
/** percentage of insert queries */
uint64_t INSERT = 80;
/** number of iterations */
uint64_t runs = 2;

unsigned int NUM_THREADS = std::thread::hardware_concurrency(); // Default to hardware concurrency

// List of valid flags and description
void validFlagsDescription()
{
    cout << "-ops: specify total number of operations\n";
    cout << "-thr=<value>: number of threads to use (e.g., -thr=4)\n";
    cout << "-rns: the number of iterations\n";
}

// Code snippet to parse command line flags and initialize the variables
int parse_args(char *arg)
{
    string s = string(arg);
    string s1;
    uint64_t val;

    try
    {
        s1 = s.substr(0, 4);
        string s2 = s.substr(5);
        val = stol(s2);
    }
    catch (...)
    {
        cout << "Supported: " << std::endl;
        cout << "-*=[], where * is:" << std::endl;
        validFlagsDescription();
        return 1;
    }

    if (s1 == "-ops")
    {
        NUM_OPS = val;
    }
    else if (s1 == "-thr")
    {
        if (val == 0)
        {
            cout << "Number of threads must be positive.\n";
            return 1;
        }
        NUM_THREADS = static_cast<unsigned int>(val);
    }
    else if (s1 == "-rns")
    {
        runs = val;
    }
    else
    {
        std::cout << "Unsupported flag:" << s1 << "\n";
        std::cout << "Use the below list flags:\n";
        validFlagsDescription();
        return 1;
    }
    return 0;
}

struct ThreadArgs
{
    BloomFilter *bf;
    const uint32_t *arr;
    uint64_t num_ops;
    int thread_id; // just in case
};

void worker_thread(ThreadArgs args)
{
    for (uint64_t i = 0; i < args.num_ops; ++i)
    {
        if (rand() % 8 == 0)
        {
            args.bf->add(args.arr[i]);
        }
        else
        {
            bool res = args.bf->contains(args.arr[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        int error = parse_args(argv[i]);
        if (error == 1)
        {
            cout << "Argument error, terminating run.\n";
            exit(EXIT_FAILURE);
        }
    }

    // Use shared files filled with random numbers
    path cwd = std::filesystem::current_path();
    path path_insert_values = cwd / "random_values_insert.bin";
    assert(std::filesystem::exists(path_insert_values));

    // Read data from file
    auto *values_insert = new uint32_t[NUM_OPS];
    read_data(path_insert_values, NUM_OPS, values_insert);

    // Max limit of the uint32_t: 4,294,967,295
    std::mt19937 gen(RANDOM_SEED);
    std::uniform_int_distribution<uint32_t> dist_int(1, NUM_OPS);

    float total_time = 0.0F;
    std::vector<std::thread> threads(NUM_THREADS);

    HRTimer start, end;
    for (uint32_t i = 0; i < runs; i++)
    {
        BloomFilter bf(1<<24);
        start = HR::now();

        //  Whether a thread issues a enq() or a deq() can be decided based on probability.
        //  issue concurrent calls to the bf
        uint64_t enq_ops = NUM_OPS / NUM_THREADS;
        uint64_t extra_enq = NUM_OPS % NUM_THREADS;

        uint32_t offset = 0;

        for (unsigned int i = 0; i < NUM_THREADS; ++i)
        {
            uint64_t thread_enq = enq_ops + (i < extra_enq ? 1 : 0);

            ThreadArgs args;
            args.bf = &bf;
            args.arr = values_insert + offset;
            args.num_ops = thread_enq;
            args.thread_id = i;

            // Debug
            // cout << "Thread " << i << ": EnQ=" << thread_enq << ", DeQ=" << thread_deq << ", Offset=" << offset << endl;

            threads[i] = std::thread(worker_thread, args);
            offset += thread_enq;
        }

        for (unsigned int i = 0; i < NUM_THREADS; ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }

        end = HR::now();
        float iter_time = duration_cast<milliseconds>(end - start).count();
        total_time += iter_time;

        cout << "Run " << (i + 1) << " completed in " << iter_time << " ms." << endl;
    }

    cout << "Time taken by kernel (ms): " << total_time / runs << "\n";

    return EXIT_SUCCESS;
}