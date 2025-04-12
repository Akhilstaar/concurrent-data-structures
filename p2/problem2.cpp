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
#include <atomic>

#ifdef USE_BOOST_QUEUE
#include <boost/lockfree/queue.hpp>
#else
#include "lockfreequeue.h"
#endif

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
uint64_t NUM_OPS = 1e6;
/** number of iterations */
uint64_t runs = 2;

unsigned int NUM_THREADS = std::thread::hardware_concurrency(); // Default to hardware concurrency

// List of valid flags and description
void validFlagsDescription()
{
    cout << "-ops=<value>: specify total number of operations (e.g., -ops=1000000)\n";
    cout << "-thr=<value>: number of threads to use (e.g., -thr=4)\n";
    cout << "-rns=<value>: the number of iterations (e.g., -rns=3)\n";
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

#ifdef USE_BOOST_QUEUE
struct ThreadArgs
{
    boost::lockfree::queue<uint32_t> *boost_queue;
    const uint32_t *insert_data_start;
    uint64_t num_ops_per_thread;
    int thread_id;
    std::atomic<uint64_t> *success_enq;
    std::atomic<uint64_t> *success_deq;
};

void worker_thread(ThreadArgs args)
{
    uint64_t local_success_enq = 0;
    uint64_t local_success_deq = 0;
    uint64_t tmp;

    for (uint64_t i = 0; i < args.num_ops_per_thread; i++)
    {
        if (rand() % 8 == 0)
        {
            if (args.boost_queue->push(args.insert_data_start[i % (args.num_ops_per_thread)])) // Use modulo to avoid out-of-bounds if data array is smaller than total ops
            {
                local_success_enq++;
            }
        }
        else
        {
            if (args.boost_queue->pop(tmp))
            {
                local_success_deq++;
            }
        }
    }
    args.success_enq->fetch_add(local_success_enq);
    args.success_deq->fetch_add(local_success_deq);
}

#else // Using custom LockFreeQueue

struct ThreadArgs
{
    LockFreeQueue *queue;
    const uint32_t *insert_data_start;
    uint64_t num_ops_per_thread;
    int thread_id;
    std::atomic<uint64_t> *success_enq;
    std::atomic<uint64_t> *success_deq;
};

void worker_thread(ThreadArgs args)
{
    uint64_t local_success_enq = 0;
    uint64_t local_success_deq = 0;

    for (uint64_t i = 0; i < args.num_ops_per_thread; i++)
    {
        if (rand() % 8 == 0)
        {
            args.queue->enq(args.insert_data_start[i % (args.num_ops_per_thread)]);
            local_success_enq++;
        }
        else
        {
            int result = args.queue->deq();
            if (result != -1)
            {
                local_success_deq++;
            }
        }
    }
    args.success_enq->fetch_add(local_success_enq);
    args.success_deq->fetch_add(local_success_deq);
}
#endif

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

#ifdef USE_BOOST_QUEUE
    cout << "Using Boost Lock-Free Queue" << endl;
#else
    cout << "Using Custom Lock-Free Queue" << endl;
#endif
    cout << "Total Ops: " << NUM_OPS << endl;
    cout << "Threads: " << NUM_THREADS << endl;
    cout << "Runs: " << runs << endl;

    path cwd = std::filesystem::current_path();
    path path_insert_values = cwd / "random_values_insert.bin";

    assert(std::filesystem::exists(path_insert_values));

    uint64_t max_possible_enqueues = NUM_OPS;
    auto *values_insert = new uint32_t[max_possible_enqueues];
    read_data(path_insert_values, max_possible_enqueues, values_insert);

    float total_time = 0.0F;
    double total_ops_executed = 0;
    uint64_t total_success_enq_all_runs = 0;
    uint64_t total_success_deq_all_runs = 0;

    HRTimer start, end;
    for (uint32_t run = 0; run < runs; run++)
    {
#ifdef USE_BOOST_QUEUE
        boost::lockfree::queue<uint32_t> queue_instance(NUM_OPS);
#else
        LockFreeQueue queue_instance;
#endif

        std::vector<std::thread> threads(NUM_THREADS);
        std::vector<ThreadArgs> thread_args(NUM_THREADS);
        std::atomic<uint64_t> run_success_enq = {0};
        std::atomic<uint64_t> run_success_deq = {0};

        uint64_t ops_per_thread = NUM_OPS / NUM_THREADS;
        uint64_t ops_remainder = NUM_OPS % NUM_THREADS;
        uint64_t current_data_offset = 0;

        start = HR::now();

        for (unsigned int i = 0; i < NUM_THREADS; i++)
        {
            uint64_t thread_ops = ops_per_thread + (i < ops_remainder ? 1 : 0);
            uint64_t data_needed = (thread_ops + 1) / 2;

#ifdef USE_BOOST_QUEUE
            thread_args[i].boost_queue = &queue_instance;
#else
            thread_args[i].queue = &queue_instance;
#endif
            thread_args[i].insert_data_start = values_insert + current_data_offset;
            thread_args[i].num_ops_per_thread = thread_ops;
            thread_args[i].thread_id = i;
            thread_args[i].success_enq = &run_success_enq;
            thread_args[i].success_deq = &run_success_deq;

            threads[i] = std::thread(worker_thread, thread_args[i]);

            current_data_offset += data_needed;
        }

        for (unsigned int i = 0; i < NUM_THREADS; i++)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }

        end = HR::now();
        float iter_time = duration_cast<milliseconds>(end - start).count();
        total_time += iter_time;

        uint64_t successful_ops_this_run = run_success_enq.load() + run_success_deq.load();
        total_success_enq_all_runs += run_success_enq.load();
        total_success_deq_all_runs += run_success_deq.load();
        total_ops_executed += successful_ops_this_run;

        cout << "Run " << (run + 1) << " completed in " << iter_time << " ms. ";
    }

    float avg_time_ms = total_time / runs;
    double avg_successful_ops = total_ops_executed / runs;
    double avg_throughput_kops_sec = 0;
    if (avg_time_ms > 0)
    {
        avg_throughput_kops_sec = (avg_successful_ops / (avg_time_ms / 1000.0)) / 1000.0;
    }

    cout << "Average time per run (ms): " << avg_time_ms << "\n";
    cout << "Average successful ENQ ops per run: " << (double)total_success_enq_all_runs / runs << "\n";
    cout << "Average successful DEQ ops per run: " << (double)total_success_deq_all_runs / runs << "\n";
    cout << "Average total successful ops per run: " << avg_successful_ops << "\n";
    cout << "Average Throughput (K ops/sec): " << avg_throughput_kops_sec << "\n";

    delete[] values_insert;
    return 0;
}