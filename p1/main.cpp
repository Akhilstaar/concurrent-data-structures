#include "hash_table.h"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <algorithm>

using namespace std;

void generate_kv_pairs(KeyValuePairs *kv_pairs, size_t n)
{
    kv_pairs->count = n;
    kv_pairs->pairs = static_cast<KeyValuePair *>(malloc(n * sizeof(KeyValuePair)));
    if (!kv_pairs->pairs)
    {
        cerr << "Failed to allocate KeyValuePairs\n";
        exit(EXIT_FAILURE);
    }

    srand(time(nullptr));
    for (size_t i = 0; i < n; ++i)
    {
        kv_pairs->pairs[i].key = static_cast<unsigned int>(rand() ^ (rand() << 16));
        kv_pairs->pairs[i].value = static_cast<unsigned int>(i + 1);
        if (kv_pairs->pairs[i].key == 0)
            kv_pairs->pairs[i].key = static_cast<unsigned int>(-1);
    }
}

void generate_key_list(KeyList *key_list, const KeyValuePairs *kv_pairs, size_t n, bool shuffle)
{
    n = min(n, kv_pairs->count);
    key_list->count = n;
    key_list->keys = static_cast<unsigned int *>(malloc(n * sizeof(unsigned int)));
    if (!key_list->keys)
    {
        cerr << "Failed to allocate KeyList\n";
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < n; ++i)
    {
        key_list->keys[i] = kv_pairs->pairs[i].key;
    }

    if (shuffle)
    {
        for (size_t i = n - 1; i > 0; --i)
        {
            size_t j = rand() % (i + 1);
            swap(key_list->keys[i], key_list->keys[j]);
        }
    }
}

void batch_insert(HashTable *ht, KeyValuePairs *kv_pairs, bool *results)
{
    size_t num_pairs = kv_pairs->count;
    vector<thread> threads;
    size_t num_threads = thread::hardware_concurrency();
    num_threads = min(num_threads, num_pairs);
    size_t chunk_size = num_pairs / num_threads;

    auto worker = [ht, kv_pairs, results](size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
        {
            results[i] = ht->insert(kv_pairs->pairs[i].key, kv_pairs->pairs[i].value);
        }
    };

    for (size_t t = 0; t < num_threads; ++t)
    {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_pairs : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads)
        t.join();
}

void batch_delete(HashTable *ht, KeyList *key_list, bool *results)
{
    size_t num_keys = key_list->count;
    vector<thread> threads;
    size_t num_threads = thread::hardware_concurrency();
    num_threads = min(num_threads, num_keys);
    size_t chunk_size = num_keys / num_threads;

    auto worker = [ht, key_list, results](size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
        {
            results[i] = ht->remove(key_list->keys[i]);
        }
    };

    for (size_t t = 0; t < num_threads; ++t)
    {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_keys : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads)
        t.join();
}

void batch_lookup(HashTable *ht, KeyList *key_list, uint32_t *results)
{
    size_t num_keys = key_list->count;
    vector<thread> threads;
    size_t num_threads = thread::hardware_concurrency();
    num_threads = min(num_threads, num_keys);
    size_t chunk_size = num_keys / num_threads;

    auto worker = [ht, key_list, results](size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
        {
            auto res = ht->get_value(key_list->keys[i]);
            results[i] = res.first ? res.second : 0;
        }
    };

    for (size_t t = 0; t < num_threads; ++t)
    {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_keys : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads)
        t.join();
}

int main(int argc, char *argv[])
{
    size_t n = 100000;
    int num_threads = 4;
    size_t capacity = 2e5;
    bool print_table = false;

    int opt;
    while ((opt = getopt(argc, argv, "n:t:c:p")) != -1)
    {
        switch (opt)
        {
        case 'n':
            n = atol(optarg);
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        case 'c':
            capacity = atol(optarg);
            break;
        case 'p':
            print_table = true;
            break;
        default:
            fprintf(stderr, "Usage: %s [-n num_operations] [-t num_threads] [-c capacity] [-p]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (capacity == 0)
        capacity = max(n * 2, static_cast<size_t>(1024));
    num_threads = max(num_threads, 1);

    cout << "Using Standard C++ Thread Implementation\n";
    cout << "Operations (n): " << n << "\nThreads: " << num_threads
         << "\nCapacity: " << capacity << "\n----------------------------\n";

    HashTable *ht = new HashTable(capacity);

    KeyValuePairs kv_data;
    generate_kv_pairs(&kv_data, n);

    KeyList key_list_lookup;
    generate_key_list(&key_list_lookup, &kv_data, n, true);

    KeyList key_list_delete;
    generate_key_list(&key_list_delete, &kv_data, n / 2, true);

    KeyList key_list_lookup_after_delete;
    generate_key_list(&key_list_lookup_after_delete, &kv_data, n, true);

    bool *insert_results = static_cast<bool *>(malloc(n * sizeof(bool)));
    uint32_t *lookup_results = static_cast<uint32_t *>(malloc(n * sizeof(uint32_t)));
    bool *delete_results = static_cast<bool *>(malloc((n / 2) * sizeof(bool)));
    uint32_t *lookup2_results = static_cast<uint32_t *>(malloc(n * sizeof(uint32_t)));

    if (!insert_results || !lookup_results || !delete_results || !lookup2_results)
    {
        perror("Allocation failed");
        free(kv_data.pairs);
        free(key_list_lookup.keys);
        free(key_list_delete.keys);
        free(key_list_lookup_after_delete.keys);
        exit(EXIT_FAILURE);
    }

    // Test Batch Insert
    cout << "\n--- Testing Batch Insert ---\n";
    auto start = chrono::high_resolution_clock::now();
    batch_insert(ht, &kv_data, insert_results);
    auto duration = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
    long long success = count(insert_results, insert_results + n, true);
    printf("Time: %.4f sec\nSuccess: %lld/%zu\n", duration, success, n);

    // Test Lookup After Insert
    cout << "\n--- Testing Batch Lookup (After Insert) ---\n";
    start = chrono::high_resolution_clock::now();
    batch_lookup(ht, &key_list_lookup, lookup_results);
    duration = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
    success = count_if(lookup_results, lookup_results + n, [](uint32_t v)
                       { return v != 0; });
    printf("Time: %.4f sec\nFound: %lld/%zu\n", duration, success, n);

    // Test Batch Delete
    cout << "\n--- Testing Batch Delete ---\n";
    start = chrono::high_resolution_clock::now();
    batch_delete(ht, &key_list_delete, delete_results);
    duration = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
    success = count(delete_results, delete_results + key_list_delete.count, true);
    printf("Time: %.4f sec\nSuccess: %lld/%zu\n", duration, success, key_list_delete.count);

    // Test Lookup After Delete
    cout << "\n--- Testing Batch Lookup (After Delete) ---\n";
    start = chrono::high_resolution_clock::now();
    batch_lookup(ht, &key_list_lookup_after_delete, lookup2_results);
    duration = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
    success = count_if(lookup2_results, lookup2_results + n, [](uint32_t v)
                       { return v != 0; });
    printf("Time: %.4f sec\nFound: %lld/%zu (Expected ~%zu)\n", duration, success, n, n / 2);

    // Cleanup
    cout << "\nCleaning up...\n";
    delete ht;
    free(kv_data.pairs);
    free(key_list_lookup.keys);
    free(key_list_delete.keys);
    free(key_list_lookup_after_delete.keys);
    free(insert_results);
    free(lookup_results);
    free(delete_results);
    free(lookup2_results);

    cout << "Done.\n";
    return 0;
}