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
#include <cmath>  

#ifndef USE_TBB
#include "hash_table.h"
#endif

#ifdef USE_TBB
#include <tbb/concurrent_hash_map.h>
using TbbHashTable = tbb::concurrent_hash_map<uint32_t, uint32_t>;
#else

class HashTable;
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
// static const uint32_t bucket_count = 1000;
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

// --- Global Variables ---
uint64_t NUM_OPS = 1e6;
uint64_t INSERT = 100;
uint64_t DELETE = 0;
uint64_t runs = 2;
uint64_t NO_THREADS = std::thread::hardware_concurrency();

void validFlagsDescription()
{
    cout << "ops: specify total number of operations\n";
    cout << "rns: the number of iterations\n";
    cout << "add: percentage of insert queries\n";
    cout << "rem: percentage of delete queries\n";
    cout << "thr: number of threads to use\n";
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
    else if (s1 == "thr")
    {
        NO_THREADS = val;
        assert(val > 0);
    }
    else if (s1 == "-rns")
    {
        runs = val;
    }
    else if (s1 == "-add")
    {
        INSERT = val;
    }
    else if (s1 == "-rem")
    {
        DELETE = val;
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

#ifdef USE_TBB
struct InsertArgs
{
    size_t start;
    size_t end;
    TbbHashTable *tbb_ht;
    KeyValue *kv_pairs;
    bool *result;
};
struct DeleteArgs
{
    size_t start;
    size_t end;
    TbbHashTable *tbb_ht;
    uint32_t *key_list;
    bool *result;
};
struct LookupArgs
{
    size_t start;
    size_t end;
    TbbHashTable *tbb_ht;
    uint32_t *key_list;
    uint32_t *result;
};
#else
struct InsertArgs
{
    size_t start;
    size_t end;
    HashTable *ht;
    KeyValue *kv_pairs;
    bool *result;
};
struct DeleteArgs
{
    size_t start;
    size_t end;
    HashTable *ht;
    uint32_t *key_list;
    bool *result;
};
struct LookupArgs
{
    size_t start;
    size_t end;
    HashTable *ht;
    uint32_t *key_list;
    uint32_t *result;
};
#endif

static void *insertWorker(void *arg)
{
    InsertArgs *wargs = static_cast<InsertArgs *>(arg);
    for (size_t i = wargs->start; i < wargs->end; ++i)
    {
#ifdef USE_TBB
        TbbHashTable::accessor acc;
        bool created = wargs->tbb_ht->insert(acc, wargs->kv_pairs[i].key);
        if (created)
        {
            acc->second = wargs->kv_pairs[i].value;
        }
        wargs->result[i] = created;
#else
        wargs->result[i] = wargs->ht->insert(wargs->kv_pairs[i].key, wargs->kv_pairs[i].value);
#endif
    }
    pthread_exit(nullptr);
    return nullptr;
}

static void *deleteWorker(void *arg)
{
    DeleteArgs *wargs = static_cast<DeleteArgs *>(arg);
    for (size_t i = wargs->start; i < wargs->end; ++i)
    {
#ifdef USE_TBB
        wargs->result[i] = wargs->tbb_ht->erase(wargs->key_list[i]);
#else
        wargs->result[i] = wargs->ht->remove(wargs->key_list[i]);
#endif
    }
    pthread_exit(nullptr);
    return nullptr;
}

static void *lookupWorker(void *arg)
{
    LookupArgs *wargs = static_cast<LookupArgs *>(arg);
    for (size_t i = wargs->start; i < wargs->end; ++i)
    {
#ifdef USE_TBB
        TbbHashTable::const_accessor c_acc;
        if (wargs->tbb_ht->find(c_acc, wargs->key_list[i]))
        {
            wargs->result[i] = c_acc->second;
        }
        else
        {
            wargs->result[i] = 0;
        }
#else
        std::pair<bool, uint32_t> find_res = wargs->ht->get_value(wargs->key_list[i]);
        wargs->result[i] = find_res.second;
#endif
    }
    pthread_exit(nullptr);
    return nullptr;
}

static void run_batch_pthreads(size_t num_items, size_t num_threads, void *(*worker_func)(void *), std::vector<void *> &args_vec)
{
    if (num_items == 0)
        return;
    num_threads = std::min(num_threads, num_items); // checks
    if (num_threads == 0)
        num_threads = 1; // at least one thread

    size_t chunk_size = num_items / num_threads;
    size_t remainder = num_items % num_threads;

    std::vector<pthread_t> threads(num_threads);
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, worker_func, args_vec[t]);
        }
        else
        {
            threads[t] = 0;
        }
    }

    for (size_t t = 0; t < num_threads; ++t)
    {
        if (threads[t] != 0)
        {
            pthread_join(threads[t], nullptr);
        }
    }
}

#ifdef USE_TBB
void batch_insert(TbbHashTable *tbb_ht, KeyValue *kv_pairs, bool *result, size_t num_pairs)
{
    if (num_pairs == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_pairs);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<InsertArgs> args(num_threads_actual);
    size_t chunk_size = num_pairs / num_threads_actual;
    size_t remainder = num_pairs % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].tbb_ht = tbb_ht;
        args[t].kv_pairs = kv_pairs;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, insertWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}

void batch_delete(TbbHashTable *tbb_ht, uint32_t *key_list, bool *result, size_t num_keys)
{
    if (num_keys == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_keys);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<DeleteArgs> args(num_threads_actual);
    size_t chunk_size = num_keys / num_threads_actual;
    size_t remainder = num_keys % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].tbb_ht = tbb_ht;
        args[t].key_list = key_list;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, deleteWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}

void batch_search(TbbHashTable *tbb_ht, uint32_t *key_list, uint32_t *result, size_t num_keys)
{
    if (num_keys == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_keys);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<LookupArgs> args(num_threads_actual);
    size_t chunk_size = num_keys / num_threads_actual;
    size_t remainder = num_keys % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].tbb_ht = tbb_ht;
        args[t].key_list = key_list;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, lookupWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}

#else

void batch_insert(HashTable *ht, KeyValue *kv_pairs, bool *result, size_t num_pairs)
{
    if (num_pairs == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_pairs);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<InsertArgs> args(num_threads_actual);
    size_t chunk_size = num_pairs / num_threads_actual;
    size_t remainder = num_pairs % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].ht = ht;
        args[t].kv_pairs = kv_pairs;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, insertWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}

void batch_delete(HashTable *ht, uint32_t *key_list, bool *result, size_t num_keys)
{
    if (num_keys == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_keys);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<DeleteArgs> args(num_threads_actual);
    size_t chunk_size = num_keys / num_threads_actual;
    size_t remainder = num_keys % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].ht = ht;
        args[t].key_list = key_list;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, deleteWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}

void batch_search(HashTable *ht, uint32_t *key_list, uint32_t *result, size_t num_keys)
{
    if (num_keys == 0)
        return;
    size_t num_threads_actual = std::min((size_t)NO_THREADS, num_keys);
    if (num_threads_actual == 0)
        num_threads_actual = 1;

    std::vector<pthread_t> threads(num_threads_actual);
    std::vector<LookupArgs> args(num_threads_actual);
    size_t chunk_size = num_keys / num_threads_actual;
    size_t remainder = num_keys % num_threads_actual;
    size_t current_start = 0;

    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        size_t current_chunk_size = chunk_size + (t < remainder ? 1 : 0);
        args[t].start = current_start;
        args[t].end = current_start + current_chunk_size;
        args[t].ht = ht;
        args[t].key_list = key_list;
        args[t].result = result;

        if (current_chunk_size > 0)
        {
            pthread_create(&threads[t], nullptr, lookupWorker, &args[t]);
        }
        else
        {
            threads[t] = 0;
        }
        current_start += current_chunk_size;
    }
    for (size_t t = 0; t < num_threads_actual; ++t)
    {
        if (threads[t] != 0)
            pthread_join(threads[t], nullptr);
    }
}
#endif // USE_TBB

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

    uint64_t ADD = NUM_OPS * (INSERT / 100.0);
    uint64_t REM = NUM_OPS * (DELETE / 100.0);
    uint64_t FIND = NUM_OPS - (ADD + REM);

#ifdef USE_TBB
    cout << "Using TBB concurrent_hash_map" << endl;
#else
    cout << "Using custom HashTable" << endl;
#endif
    cout << "Threads: " << NO_THREADS << " Runs: " << runs << "\n";
    cout << "NUM OPS: " << NUM_OPS << " ADD: " << ADD << " REM: " << REM
         << " FIND: " << FIND << "\n";

    KeyValue *h_kvs_insert = nullptr;
    uint32_t *h_keys_del = nullptr;
    uint32_t *h_keys_lookup = nullptr;
    bool *add_result = nullptr;
    bool *del_result = nullptr;
    uint32_t *find_result = nullptr;

    if (ADD > 0)
    {
        h_kvs_insert = new KeyValue[ADD];
        memset(h_kvs_insert, 0, sizeof(KeyValue) * ADD);
        add_result = new bool[ADD];
    }
    if (REM > 0)
    {
        h_keys_del = new uint32_t[REM];
        memset(h_keys_del, 0, sizeof(uint32_t) * REM);
        del_result = new bool[REM];
    }
    if (FIND > 0)
    {
        h_keys_lookup = new uint32_t[FIND];
        memset(h_keys_lookup, 0, sizeof(uint32_t) * FIND);
        find_result = new uint32_t[FIND];
    }

    // Use shared files filled with random numbers
    path cwd = std::filesystem::current_path();
    path path_insert_keys = cwd / "random_keys_insert.bin";
    path path_insert_values = cwd / "random_values_insert.bin";
    path path_delete = cwd / "random_keys_delete.bin";
    path path_search = cwd / "random_keys_search.bin";

    assert(std::filesystem::exists(path_insert_keys));
    assert(std::filesystem::exists(path_insert_values));
    assert(std::filesystem::exists(path_delete));
    assert(std::filesystem::exists(path_search));

    // Read data from file
    auto *tmp_keys_insert = new uint32_t[ADD];
    auto *tmp_values_insert = new uint32_t[ADD];
    read_data(path_insert_keys, ADD, tmp_keys_insert);
    read_data(path_insert_values, ADD, tmp_values_insert);
    for (uint64_t i = 0; i < ADD; i++)
    {
        h_kvs_insert[i].key = tmp_keys_insert[i];
        h_kvs_insert[i].value = tmp_values_insert[i];
    }
    delete[] tmp_keys_insert;
    delete[] tmp_values_insert;

    if (REM > 0)
    {
        read_data(path_delete, REM, h_keys_del);
    }
    if (FIND > 0)
    {
        read_data(path_search, FIND, h_keys_lookup);
    }

    float total_insert_time = 0.0F;
    float total_delete_time = 0.0F;
    float total_search_time = 0.0F;
    HRTimer start, end;
    uint32_t del_runs = 0, search_runs = 0;

    for (uint32_t i = 0; i < runs; i++)
    {
#ifdef USE_TBB
        TbbHashTable *the_hash_table = new TbbHashTable();
#else
        size_t capacity = 2e5; // initial capacity
        HashTable *the_hash_table = new HashTable(capacity);
#endif

        if (ADD > 0)
        {
            memset(add_result, 0, sizeof(bool) * ADD);
            start = HR::now();
            batch_insert(the_hash_table, h_kvs_insert, add_result, ADD);
            end = HR::now();
            float iter_insert_time = duration_cast<milliseconds>(end - start).count();
            total_insert_time += iter_insert_time;
        }

        if (REM > 0)
        {
            memset(del_result, 0, sizeof(bool) * REM);
            start = HR::now();
            batch_delete(the_hash_table, h_keys_del, del_result, REM);
            end = HR::now();
            float iter_delete_time = duration_cast<milliseconds>(end - start).count();
            total_delete_time += iter_delete_time;
            del_runs++;
        }

        if (FIND > 0)
        {
            memset(find_result, 0, sizeof(uint32_t) * FIND);
            start = HR::now();
            batch_search(the_hash_table, h_keys_lookup, find_result, FIND);
            end = HR::now();
            float iter_search_time = duration_cast<milliseconds>(end - start).count();
            total_search_time += iter_search_time;
            search_runs++;
        }

        cout << "Run " << (i + 1) << " completed." << endl;
    }

    cout << "\n--- Average Times ---" << endl;
    if (runs > 0 && ADD > 0)
    {
        cout << "Avg time taken by insert kernel (ms): " << total_insert_time / runs << "\n";
    }
    if (del_runs > 0)
    {
        cout << "Avg time taken by delete kernel (ms): " << total_delete_time / del_runs << "\n";
    }
    if (search_runs > 0)
    {
        cout << "Avg time taken by search kernel (ms): " << total_search_time / search_runs << "\n";
    }

    delete[] h_kvs_insert;
    delete[] h_keys_del;
    delete[] h_keys_lookup;
    delete[] add_result;
    delete[] del_result;
    delete[] find_result;
    return 0;
}