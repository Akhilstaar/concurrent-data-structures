#include <atomic>
#include <vector>

#define FILTER_SIZE (uint32_t)1e24

class BloomFilter{
private:
    std::atomic_flag *bits;

    uint32_t hash1(uint32_t x){
        return (x ^ (x >> 16)) % FILTER_SIZE;
    }

    uint32_t hash2(uint32_t x){
        return (x ^ (x >> 16)) % FILTER_SIZE;
    }

    uint32_t hash3(uint32_t x){
        return (x ^ (x >> 16)) % FILTER_SIZE;
    }

public:
    BloomFilter(int size) {
        bits = new std::atomic_flag[size];

        // is it already initialized to 0 ?
        for (int i = 0; i < size; ++i) {
            bits[i].clear();
        }
    }

    ~BloomFilter() {
        delete[] bits;
    }

    void add(int v);
    bool contains(int v);
};