#pragma once

#include <cmath>
#include <utility>
#include <glm/glm.hpp>

namespace lns {

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    size_t jacobi(A<T>& a, X<T>& x, B<T> b, double threshold) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X<T> xOld;
        size_t k = 0;
        double error = 1;
        while(error > threshold){
            xOld = x;
            for(size_t i = 0; i < rows; i++){
                T sum{};
                for(size_t j = 0; j < cols; j++){
                    sum += T(i != j) * a[i][j] * xOld[j];
                }
                x[i] = 1.0/a[i][i] * (b[i] - sum);
            }
            error = std::abs(x[1] - xOld[1]);
            k++;
        }

         return k;
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    double jacobi(A<T>& a, X<T>& x, B<T> b, size_t iterations) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X<T> xOld;
        for(size_t k = 0; k < iterations; k++){
            xOld = x;
            for(size_t i = 0; i < rows; i++){
                T sum{};
                for(size_t j = 0; j < cols; j++){
                    sum += T(i != j) * a[i][j] * xOld[j];
                }
                x[i] = 1.0/a[i][i] * (b[i] - sum);
            }
        }

        return std::abs(x[0] - xOld[0]);
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    double gauss_seidel(A<T>& a, X<T>& x, B<T> b, size_t iterations) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X<T> xOld = x;
        double error = 0;
        for(size_t k = 0; k < iterations; k++){
            auto ox = xOld[0];
            for(size_t i = 0; i < rows; i++){
                T sum{};
                x[i] = (b[i] / a[i][i]);
                for(size_t j = 0; j < cols; j++){
                    if( i == j) continue;
                    x[i] = x[i] - (a[i][j] / a[i][i]) * xOld[j];
                    xOld[i] = x[i];
                }
                error = std::abs(x[0] - ox);
            }
        }

        return error;
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    size_t gauss_seidel(A<T>& a, X<T>& x, B<T> b, double threshold) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X<T> xOld = x;
        std::vector<double> errors;
        size_t k = 0;
        double error = 1;
        while(error > threshold){
            auto ox = xOld[0];
            for(size_t i = 0; i < rows; i++){
                x[i] = (b[i] / a[i][i]);
                for(size_t j = 0; j < cols; j++){
                    if( i == j) continue;
                    x[i] = x[i] - (a[i][j] / a[i][i]) * xOld[j];
                    xOld[i] = x[i];
                }
                error = std::abs(x[0] - ox);
            }
            k++;
        }
        return k;
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    size_t gradient_descent(A<T>& a, X<T>& x, B<T> b, double threshold, size_t maxIterations = 100) {
        X<T> xi = x;

        int k = 0;
        auto r = b - a * xi;
        auto rr = r.dot(r);
        auto epsilon = threshold * threshold * rr;
        while(k < maxIterations and rr > epsilon){
            auto q = a * r;
            auto alpha = rr / r.dot(q);

            x = xi + r * alpha;
            xi = x;
            if(k % 50 == 0){
                r = b - a * xi;
            }else {
                r = r - alpha * q;
            }
            rr = r.dot(r);
            k++;
        }

        return k;
    }

    struct IdentityPreconditioner {
        template<typename T>
        blas::MatrixT<T> operator()(blas::MatrixT<T>& matrix) {
            auto [_, size] = matrix.size();
            return blas::Matrix::identity<T>(size);
        }
    };

    template<
        typename T,
        template<typename> typename A,
        template<typename> typename X,
        template<typename> typename B,
        typename Preconditioner>
    size_t conjugate_gradient(A<T>& a, X<T>& x, B<T> b, double threshold, Preconditioner&& preconditioner) {
        const auto maxIterations = x.size();
        auto invM = preconditioner(a);
        auto xi = x;
        auto r = b - a * xi;
        auto d = invM * r;
        auto rd = r.dot(d);
        auto rd0 = rd;
        auto epsilon = threshold * threshold * rd0;

        size_t k = 0;
        while(k < maxIterations && rd > epsilon) {
            auto q = a * d;
            auto alpha = rd/d.dot(q);
            x = xi + alpha * d;
            xi = x;
            if(k % 50 == 0){
                r = b - a * xi;
            }else {
                r = r - alpha * q;
            }

            auto s = invM * r;
            rd0 = rd;
            rd = r.dot(s);
            auto beta = rd/rd0;
            d = s + beta * d;
            k++;

        }

        return k;
    }



//    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
//    size_t conjugate_gradient(A<T>& a, X<T>& x, B<T> b, double threshold) {
////        const auto maxIterations = x.size();
////        auto xi = x;
////        auto r = b - a * xi;
////        auto d = r;
////        auto rr = r.dot(r);
////        auto epsilon = threshold * threshold * rr;
////
////        int k = 0;
////        while(k < maxIterations and rr > epsilon) {
////           auto q = a * d;
////           auto alpha = rr/d.dot(q);
////            x = xi + alpha * d;
////            xi = x;
////            if(k % 50 == 0){
////                r = b - a * xi;
////            }else {
////                r = r - alpha * q;
////            }
////            auto rr0 = rr;
////            rr = r.dot(r);
////            auto beta = rr/rr0;
////            d = r + beta * d;
////            k++;
////        }
////
////        return k;
//        return conjugate_gradient<IdentityPreconditioner>(a, x, b, threshold);
//    }

}