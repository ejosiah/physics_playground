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
            error = std::abs(x[0] - xOld[0]);
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
    size_t gradient_descent(A<T>& a, X<T>& x, B<T> b, double threshold) {
        double error = 1;
        X<T> xi = x;

        int k = 0;
        auto r = b - a * xi;
        auto tt = threshold * threshold;
        while(error > tt){
            auto ar = a * r;
            auto alpha = r.dot(r);
            alpha /= r.dot(ar);

            x = xi + r * alpha;
            error = (x - xi).squaredLength();
            xi = x;
            r = r - alpha * ar;
            k++;
        }

        return k;
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    size_t conjugate_gradient(A<T>& a, X<T>& x, B<T> b, double threshold) {
        double error = 1;

        auto x0 = x;
        auto r0 = b - a * x0;
        auto d = r0;
        auto tt = threshold * threshold;

        int k = 0;
        while(error > tt) {
            auto rr = r0.squaredLength();
            auto alpha = rr / d.dot(a * d);
            x = x0 + alpha * d;
            auto r = r0 - alpha * (a * d);

            auto beta = r.dot(r) / rr;
            d = r + beta * d;

            error = (x - x0).squaredLength();
            x0 = x;
            r0 = r;
            k++;
        }

        return k;
    }

    template<typename T, template<typename> typename A, template<typename> typename X, template<typename> typename B>
    size_t conjugate_gradient(A<T>& a, X<T>& x, B<T> b) {
        double error = 1;

        auto x0 = x;
        auto r0 = b - a * x0;
        auto d = r0;

        const auto N = x.size();
        for(int k = 0; k < N; k++) {
            auto rr = r0.squaredLength();
            auto alpha = rr / d.dot(a * d);
            x = x0 + alpha * d;
            auto r = r0 - alpha * (a * d);

            auto beta = r.dot(r) / rr;
            d = r + beta * d;

            error = (x - x0).length();
            x0 = x;
            r0 = r;
        }
        return N;
    }

}