#include "bloomfilter.h"
#include <atomic>

void BloomFilter::add(int v)
{
    bits[hash1(v)].test_and_set(std::memory_order_release);
    bits[hash2(v)].test_and_set(std::memory_order_release);
    bits[hash3(v)].test_and_set(std::memory_order_release);
}

bool BloomFilter::contains(int v)
{
    return (bits[hash1(v)].test(std::memory_order_acquire) && bits[hash2(v)].test(std::memory_order_acquire) && bits[hash3(v)].test(std::memory_order_acquire));
}