#ifndef MYPOINTER_INT_PAIR_H
#define MYPOINTER_INT_PAIR_H

#include <cstdint>
#include <cassert>
#include <iostream>

// works only for 64bit systems.
template <typename Nodeptr>
class MyPointerIntPair {
private:
    uintptr_t Value;  // Use uintptr_t to store pointer-sized integer

public:
    MyPointerIntPair() : Value(0) {}

    MyPointerIntPair(Nodeptr PtrVal, uint16_t IntVal) {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(PtrVal);
        assert((ptr >> 48) == 0 && "Top 16 bits should be 0"); // just in case 
        Value = (ptr & ((static_cast<uintptr_t>(1) << 48 )- 1)) | (static_cast<uintptr_t>(IntVal) << 48);
    }

    Nodeptr getPtr() const {
        return reinterpret_cast<Nodeptr>(Value & ((static_cast<uintptr_t>(1) << 48) - 1));
    }

    uint16_t getCnt() const {
        return static_cast<uint16_t>(Value >> 48);
    }

    friend bool operator==(const MyPointerIntPair& a, const MyPointerIntPair& b) {
        return a.Value == b.Value;
    }

    friend bool operator!=(const MyPointerIntPair& a, const MyPointerIntPair& b) {
        return a.Value != b.Value;
    }
};

#endif