#include <bits/stdc++.h>



/// A traits type that is used to handle pointer types and things that are just
/// wrappers for pointers as a uniform entity.
template <typename T>
struct PointerLikeTypeTraits;

namespace detail
{
    /// A tiny meta function to compute the log2 of a compile time constant.
    template <size_t N>
    struct ConstantLog2
        : std::integral_constant<size_t, ConstantLog2<N / 2>::value + 1>
    {
    };
    template <>
    struct ConstantLog2<1> : std::integral_constant<size_t, 0>
    {
    };

    // Provide a trait to check if T is pointer-like.
    template <typename T, typename U = void>
    struct HasPointerLikeTypeTraits
    {
        static const bool value = false;
    };

    // sizeof(T) is valid only for a complete T.
    template <typename T>
    struct HasPointerLikeTypeTraits<
        T, decltype((sizeof(PointerLikeTypeTraits<T>) + sizeof(T)), void())>
    {
        static const bool value = true;
    };

    template <typename T>
    struct IsPointerLike
    {
        static const bool value = HasPointerLikeTypeTraits<T>::value;
    };

    template <typename T>
    struct IsPointerLike<T *>
    {
        static const bool value = true;
    };
} // namespace detail

// Provide PointerLikeTypeTraits for non-cvr pointers.
template <typename T>
struct PointerLikeTypeTraits<T *>
{
    static inline void *getAsVoidPointer(T *P) { return P; }
    static inline T *getFromVoidPointer(void *P) { return static_cast<T *>(P); }

    static constexpr int NumLowBitsAvailable =
        detail::ConstantLog2<alignof(T)>::value;
};

template <>
struct PointerLikeTypeTraits<void *>
{
    static inline void *getAsVoidPointer(void *P) { return P; }
    static inline void *getFromVoidPointer(void *P) { return P; }

    /// Note, we assume here that void* is related to raw malloc'ed memory and
    /// that malloc returns objects at least 4-byte aligned. However, this may be
    /// wrong, or pointers may be from something other than malloc. In this case,
    /// you should specify a real typed pointer or avoid this template.
    ///
    /// All clients should use assertions to do a run-time check to ensure that
    /// this is actually true.
    static constexpr int NumLowBitsAvailable = 2;
};

// Provide PointerLikeTypeTraits for const things.
template <typename T>
struct PointerLikeTypeTraits<const T>
{
    typedef PointerLikeTypeTraits<T> NonConst;

    static inline const void *getAsVoidPointer(const T P)
    {
        return NonConst::getAsVoidPointer(P);
    }
    static inline const T getFromVoidPointer(const void *P)
    {
        return NonConst::getFromVoidPointer(const_cast<void *>(P));
    }
    static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};

// Provide PointerLikeTypeTraits for const pointers.
template <typename T>
struct PointerLikeTypeTraits<const T *>
{
    typedef PointerLikeTypeTraits<T *> NonConst;

    static inline const void *getAsVoidPointer(const T *P)
    {
        return NonConst::getAsVoidPointer(const_cast<T *>(P));
    }
    static inline const T *getFromVoidPointer(const void *P)
    {
        return NonConst::getFromVoidPointer(const_cast<void *>(P));
    }
    static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};

// Provide PointerLikeTypeTraits for uintptr_t.
template <>
struct PointerLikeTypeTraits<uintptr_t>
{
    static inline void *getAsVoidPointer(uintptr_t P)
    {
        return reinterpret_cast<void *>(P);
    }
    static inline uintptr_t getFromVoidPointer(void *P)
    {
        return reinterpret_cast<uintptr_t>(P);
    }
    // No bits are available!
    static constexpr int NumLowBitsAvailable = 0;
};

/// Provide suitable custom traits struct for function pointers.
///
/// Function pointers can't be directly given these traits as functions can't
/// have their alignment computed with `alignof` and we need different casting.
///
/// To rely on higher alignment for a specialized use, you can provide a
/// customized form of this template explicitly with higher alignment, and
/// potentially use alignment attributes on functions to satisfy that.
template <int Alignment, typename FunctionPointerT>
struct FunctionPointerLikeTypeTraits
{
    static constexpr int NumLowBitsAvailable =
        detail::ConstantLog2<Alignment>::value;
    static inline void *getAsVoidPointer(FunctionPointerT P)
    {
        assert((reinterpret_cast<uintptr_t>(P) &
                ~((uintptr_t)-1 << NumLowBitsAvailable)) == 0 &&
               "Alignment not satisfied for an actual function pointer!");
        return reinterpret_cast<void *>(P);
    }
    static inline FunctionPointerT getFromVoidPointer(void *P)
    {
        return reinterpret_cast<FunctionPointerT>(P);
    }
};

/// Provide a default specialization for function pointers that assumes 4-byte
/// alignment.
///
/// We assume here that functions used with this are always at least 4-byte
/// aligned. This means that, for example, thumb functions won't work or systems
/// with weird unaligned function pointers won't work. But all practical systems
/// we support satisfy this requirement.
template <typename ReturnT, typename... ParamTs>
struct PointerLikeTypeTraits<ReturnT (*)(ParamTs...)>
    : FunctionPointerLikeTypeTraits<4, ReturnT (*)(ParamTs...)>
{
};

namespace detail
{
    template <typename Ptr>
    struct PunnedPointer
    {
        static_assert(sizeof(Ptr) == sizeof(intptr_t), "");

        // Asserts that allow us to let the compiler implement the destructor and
        // copy/move constructors
        static_assert(std::is_trivially_destructible<Ptr>::value, "");
        static_assert(std::is_trivially_copy_constructible<Ptr>::value, "");
        static_assert(std::is_trivially_move_constructible<Ptr>::value, "");

        explicit constexpr PunnedPointer(intptr_t i = 0) { *this = i; }

        constexpr intptr_t asInt() const
        {
            intptr_t R = 0;
            std::memcpy(&R, Data, sizeof(R));
            return R;
        }

        constexpr operator intptr_t() const { return asInt(); }

        constexpr PunnedPointer &operator=(intptr_t V)
        {
            std::memcpy(Data, &V, sizeof(Data));
            return *this;
        }

        Ptr *getPointerAddress() { return reinterpret_cast<Ptr *>(Data); }
        const Ptr *getPointerAddress() const { return reinterpret_cast<Ptr *>(Data); }

    private:
        alignas(Ptr) unsigned char Data[sizeof(Ptr)];
    };
} // namespace detail

template <typename T, typename Enable>
struct DenseMapInfo;
template <typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo;

/// PointerIntPair - This class implements a pair of a pointer and small
/// integer.  It is designed to represent this in the space required by one
/// pointer by bitmangling the integer into the low part of the pointer.  This
/// can only be done for small integers: typically up to 3 bits, but it depends
/// on the number of bits available according to PointerLikeTypeTraits for the
/// type.
///
/// Note that PointerIntPair always puts the IntVal part in the highest bits
/// possible.  For example, PointerIntPair<void*, 1, bool> will put the bit for
/// the bool into bit #2, not bit #0, which allows the low two bits to be used
/// for something else.  For example, this allows:
///   PointerIntPair<PointerIntPair<void*, 1, bool>, 1, bool>
/// ... and the two bools will land in different bits.
template <typename PointerTy, unsigned IntBits, typename IntType = unsigned,
          typename PtrTraits = PointerLikeTypeTraits<PointerTy>,
          typename Info = PointerIntPairInfo<PointerTy, IntBits, PtrTraits>>
class PointerIntPair
{
    // Used by MSVC visualizer and generally helpful for debugging/visualizing.
    using InfoTy = Info;
    detail::PunnedPointer<PointerTy> Value;

public:
    constexpr PointerIntPair() = default;

    PointerIntPair(PointerTy PtrVal, IntType IntVal)
    {
        setPointerAndInt(PtrVal, IntVal);
    }

    explicit PointerIntPair(PointerTy PtrVal) { initWithPointer(PtrVal); }

    PointerTy getPointer() const { return Info::getPointer(Value); }

    IntType getInt() const { return (IntType)Info::getInt(Value); }

    void setPointer(PointerTy PtrVal) &
    {
        Value = Info::updatePointer(Value, PtrVal);
    }

    void setInt(IntType IntVal) &
    {
        Value = Info::updateInt(Value, static_cast<intptr_t>(IntVal));
    }

    void initWithPointer(PointerTy PtrVal) &
    {
        Value = Info::updatePointer(0, PtrVal);
    }

    void setPointerAndInt(PointerTy PtrVal, IntType IntVal) &
    {
        Value = Info::updateInt(Info::updatePointer(0, PtrVal),
                                static_cast<intptr_t>(IntVal));
    }

    PointerTy const *getAddrOfPointer() const
    {
        return const_cast<PointerIntPair *>(this)->getAddrOfPointer();
    }

    PointerTy *getAddrOfPointer()
    {
        assert(Value == reinterpret_cast<intptr_t>(getPointer()) &&
               "Can only return the address if IntBits is cleared and "
               "PtrTraits doesn't change the pointer");
        return Value.getPointerAddress();
    }

    void *getOpaqueValue() const
    {
        return reinterpret_cast<void *>(Value.asInt());
    }

    void setFromOpaqueValue(void *Val) &
    {
        Value = reinterpret_cast<intptr_t>(Val);
    }

    static PointerIntPair getFromOpaqueValue(void *V)
    {
        PointerIntPair P;
        P.setFromOpaqueValue(V);
        return P;
    }

    // Allow PointerIntPairs to be created from const void * if and only if the
    // pointer type could be created from a const void *.
    static PointerIntPair getFromOpaqueValue(const void *V)
    {
        (void)PtrTraits::getFromVoidPointer(V);
        return getFromOpaqueValue(const_cast<void *>(V));
    }

    bool operator==(const PointerIntPair &RHS) const
    {
        return Value == RHS.Value;
    }

    bool operator!=(const PointerIntPair &RHS) const
    {
        return Value != RHS.Value;
    }

    bool operator<(const PointerIntPair &RHS) const { return Value < RHS.Value; }
    bool operator>(const PointerIntPair &RHS) const { return Value > RHS.Value; }

    bool operator<=(const PointerIntPair &RHS) const
    {
        return Value <= RHS.Value;
    }

    bool operator>=(const PointerIntPair &RHS) const
    {
        return Value >= RHS.Value;
    }
};

template <typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo
{
    static_assert(PtrTraits::NumLowBitsAvailable <
                      std::numeric_limits<uintptr_t>::digits,
                  "cannot use a pointer type that has all bits free");
    static_assert(IntBits <= PtrTraits::NumLowBitsAvailable,
                  "PointerIntPair with integer size too large for pointer");
    enum MaskAndShiftConstants : uintptr_t
    {
        /// PointerBitMask - The bits that come from the pointer.
        PointerBitMask =
            ~(uintptr_t)(((intptr_t)1 << PtrTraits::NumLowBitsAvailable) - 1),

        /// IntShift - The number of low bits that we reserve for other uses, and
        /// keep zero.
        IntShift = (uintptr_t)PtrTraits::NumLowBitsAvailable - IntBits,

        /// IntMask - This is the unshifted mask for valid bits of the int type.
        IntMask = (uintptr_t)(((intptr_t)1 << IntBits) - 1),

        // ShiftedIntMask - This is the bits for the integer shifted in place.
        ShiftedIntMask = (uintptr_t)(IntMask << IntShift)
    };

    static PointerT getPointer(intptr_t Value)
    {
        return PtrTraits::getFromVoidPointer(
            reinterpret_cast<void *>(Value & PointerBitMask));
    }

    static intptr_t getInt(intptr_t Value)
    {
        return (Value >> IntShift) & IntMask;
    }

    static intptr_t updatePointer(intptr_t OrigValue, PointerT Ptr)
    {
        intptr_t PtrWord =
            reinterpret_cast<intptr_t>(PtrTraits::getAsVoidPointer(Ptr));
        assert((PtrWord & ~PointerBitMask) == 0 &&
               "Pointer is not sufficiently aligned");
        // Preserve all low bits, just update the pointer.
        return PtrWord | (OrigValue & ~PointerBitMask);
    }

    static intptr_t updateInt(intptr_t OrigValue, intptr_t Int)
    {
        intptr_t IntWord = static_cast<intptr_t>(Int);
        assert((IntWord & ~IntMask) == 0 && "Integer too large for field");

        // Preserve all bits other than the ones we are updating.
        return (OrigValue & ~ShiftedIntMask) | IntWord << IntShift;
    }
};

// Provide specialization of DenseMapInfo for PointerIntPair.
template <typename PointerTy, unsigned IntBits, typename IntType>
struct DenseMapInfo<PointerIntPair<PointerTy, IntBits, IntType>, void>
{
    using Ty = PointerIntPair<PointerTy, IntBits, IntType>;

    static Ty getEmptyKey()
    {
        uintptr_t Val = static_cast<uintptr_t>(-1);
        Val <<= PointerLikeTypeTraits<Ty>::NumLowBitsAvailable;
        return Ty::getFromOpaqueValue(reinterpret_cast<void *>(Val));
    }

    static Ty getTombstoneKey()
    {
        uintptr_t Val = static_cast<uintptr_t>(-2);
        Val <<= PointerLikeTypeTraits<PointerTy>::NumLowBitsAvailable;
        return Ty::getFromOpaqueValue(reinterpret_cast<void *>(Val));
    }

    static unsigned getHashValue(Ty V)
    {
        uintptr_t IV = reinterpret_cast<uintptr_t>(V.getOpaqueValue());
        return unsigned(IV) ^ unsigned(IV >> 9);
    }

    static bool isEqual(const Ty &LHS, const Ty &RHS) { return LHS == RHS; }
};

// Teach SmallPtrSet that PointerIntPair is "basically a pointer".
template <typename PointerTy, unsigned IntBits, typename IntType,
          typename PtrTraits>
struct PointerLikeTypeTraits<
    PointerIntPair<PointerTy, IntBits, IntType, PtrTraits>>
{
    static inline void *
    getAsVoidPointer(const PointerIntPair<PointerTy, IntBits, IntType> &P)
    {
        return P.getOpaqueValue();
    }

    static inline PointerIntPair<PointerTy, IntBits, IntType>
    getFromVoidPointer(void *P)
    {
        return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(P);
    }

    static inline PointerIntPair<PointerTy, IntBits, IntType>
    getFromVoidPointer(const void *P)
    {
        return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(P);
    }

    static constexpr int NumLowBitsAvailable =
        PtrTraits::NumLowBitsAvailable - IntBits;
};

// Allow structured bindings on PointerIntPair.
template <std::size_t I, typename PointerTy, unsigned IntBits, typename IntType,
          typename PtrTraits, typename Info>
decltype(auto)
get(const PointerIntPair<PointerTy, IntBits, IntType, PtrTraits, Info> &Pair)
{
    static_assert(I < 2);
    if constexpr (I == 0)
        return Pair.getPointer();
    else
        return Pair.getInt();
}