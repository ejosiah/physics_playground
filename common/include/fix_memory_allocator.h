#pragma once

#include <cstddef>
#include <vector>

template<typename T>
class FixedMemoryAllocatorT {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment  = std::true_type;
    using is_always_equal  = std::true_type;

public:
    FixedMemoryAllocatorT() = default;

    FixedMemoryAllocatorT(size_t size) : _memory(size) {
        allocationPtr = _memory.data();
    }

    [[nodiscard]] constexpr T* allocate( std::size_t n ) {
        auto allocation = allocationPtr;
        allocationPtr += n;
        return allocation;
    }

    constexpr void deallocate( T* p, std::size_t n ) {
        // Ignore
    }

private:
    std::vector<T> _memory;
    T* allocationPtr{nullptr};
};

template<size_t Size>
using FixedMemoryAllocator = FixedMemoryAllocatorT<float>;