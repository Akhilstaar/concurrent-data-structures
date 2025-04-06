#ifndef USE_TBB

#include "hash_table.h"
#include <iostream>

void batch_insert(HashTable *ht, KeyValuePairs *kv_pairs, bool *result)
{
    size_t num_pairs = kv_pairs->count;
    std::vector<std::thread> threads;

    // worker function
    auto worker = [ht, kv_pairs, result](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            result[i] = ht->insert(kv_pairs->pairs[i].key, kv_pairs->pairs[i].value);
        }
    };

    size_t num_threads = std::thread::hardware_concurrency();
    size_t chunk_size = num_pairs / num_threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_pairs : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads) {
        t.join();
    }
    return;
}

void batch_delete(HashTable *ht, KeyList *key_list, bool *result)
{
    size_t num_pairs = key_list->count;
    std::vector<std::thread> threads;

    // worker function
    auto worker = [ht, key_list, result](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            result[i] = ht->remove(key_list->keys[i]);
        }
    };

    size_t num_threads = std::thread::hardware_concurrency();
    size_t chunk_size = num_pairs / num_threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_pairs : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads) {
        t.join();
    }
    return;
}

void batch_lookup(HashTable *ht, KeyList *key_list, uint32_t *result)
{
    size_t num_pairs = key_list->count;
    std::vector<std::thread> threads;

    // worker function
    auto worker = [ht, key_list, result](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            result[i] = ht->get_value(key_list->keys[i]).second;
        }
    };

    size_t num_threads = std::thread::hardware_concurrency();
    size_t chunk_size = num_pairs / num_threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = (t == num_threads - 1) ? num_pairs : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads) {
        t.join();
    }
    return;
}
#endif