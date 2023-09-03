#pragma once

#include <memory>

namespace blas {

    template<typename T, typename Allocator = std::allocator<T>>
    class VectorT;

    template<typename T>
    class SparseVectorT;

    template<typename T>
    class MatrixT;

    template<typename T>
    class SparseMatrixT;
}