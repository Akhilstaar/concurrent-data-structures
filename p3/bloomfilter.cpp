#include "bloomfilter.h"
#include <atomic>

void BloomFilter::add(int v)
{
    bits[hash1(v)].test_and_set();
    bits[hash2(v)].test_and_set();
    bits[hash3(v)].test_and_set();
}

bool BloomFilter::contains(int v)
{
    return (bits[hash1(v)].test() && bits[hash2(v)].test() && bits[hash3(v)].test());
}