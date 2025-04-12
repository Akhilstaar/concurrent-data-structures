// bloomfilter.h
#include <atomic>
#include <vector>

#define FILTER_SIZE (uint32_t)1e24

class BloomFilter
{
private:
    std::atomic_flag *bits;

    uint64_t hash1(uint32_t x) const
    {
        // Simple Fowler-Noll-Vo (FNV-1a) basis
        uint64_t hash = 14695981039346656037ULL;     // FNV offset basis
        const uint64_t fnv_prime = 1099511627776ULL; // FNV prime
        unsigned char *bytes = reinterpret_cast<unsigned char *>(&x);
        for (int i = 0; i < sizeof(uint32_t); ++i)
        {
            hash ^= (uint64_t)bytes[i];
            hash *= fnv_prime;
        }
        return hash % FILTER_SIZE;
    }

    uint64_t hash2(uint32_t x) const
    {
        uint64_t hash = (static_cast<uint64_t>(x) * 31) ^ (static_cast<uint64_t>(x) >> 15);
        return hash % FILTER_SIZE;
    }

    uint64_t hash3(uint32_t x) const
    {
        uint64_t hash = (static_cast<uint64_t>(x) * 17) ^ (static_cast<uint64_t>(x) << 5);
        return hash % FILTER_SIZE;
    }

public:
    BloomFilter(uint64_t size_in_bits)
    {
        bits = new std::atomic_flag[FILTER_SIZE];

        for (uint64_t i = 0; i < FILTER_SIZE; ++i)
        {
            bits[i].clear(std::memory_order_relaxed);
        }
    }

    ~BloomFilter()
    {
        delete[] bits;
    }

    void add(int v);
    bool contains(int v);
};