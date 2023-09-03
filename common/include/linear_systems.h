#pragma once

#include <cmath>
#include <utility>
#include <glm/glm.hpp>
#include <iostream>
#include <format>
#include <chrono>

namespace lns {

    template<typename A, typename X, typename B>
    size_t jacobi(const A& a, X& x, const B& b, double threshold) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X xOld;
        size_t k = 0;
        double error = 1;
        auto t = x[0];
        while(error > threshold){
            xOld = x;
            for(size_t i = 0; i < rows; i++){
                decltype(t) sum{0};
                for(size_t j = 0; j < cols; j++){
                    sum += (i != j) * a[i][j] * xOld[j];
                }
                x[i] = 1.0/a[i][i] * (b[i] - sum);
            }
            auto err = x - xOld;
            error = *std::max_element(err.begin(), err.end());
            k++;
        }

         return k;
    }

    template<typename A, typename X, typename B>
    void jacobi(const A& a, X& x, const B b, size_t iterations) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X xOld;
        auto t = x[0];
        for(size_t k = 0; k < iterations; k++){
            xOld = x;
            for(size_t i = 0; i < rows; i++){
                decltype(t) sum{0};
                for(size_t j = 0; j < cols; j++){
                    sum += (i != j) * a[i][j] * xOld[j];
                }
                x[i] = 1.0/a[i][i] * (b[i] - sum);
            }
        }
    }

    template<typename A, typename X, typename B>
    void gauss_seidel(const A& a, X& x, const B b, size_t iterations) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X xOld = x;
        auto t = x[0];
        for(size_t k = 0; k < iterations; k++){
            auto ox = xOld[0];
            for(size_t i = 0; i < rows; i++){
                decltype(t) sum{};
                x[i] = (b[i] / a[i][i]);
                for(size_t j = 0; j < cols; j++){
                    if( i == j) continue;
                    x[i] = x[i] - (a[i][j] / a[i][i]) * xOld[j];
                    xOld[i] = x[i];
                }
            }
        }
    }

    template<typename A, typename X, typename B>
    size_t gauss_seidel(const A& a, X& x, const B b, double threshold) {
        const auto [cols, _] = a.size();
        const size_t rows = b.size();
        X xOld = x;
        size_t k = 0;
        auto sentinel = false;
        while(!sentinel){
            auto ox = xOld;
            for(size_t i = 0; i < rows; i++){
                x[i] = (b[i] / a[i][i]);
                for(size_t j = 0; j < cols; j++){
                    if( i == j) continue;
                    x[i] = x[i] - (a[i][j] / a[i][i]) * xOld[j];
                    xOld[i] = x[i];
                }
                auto err = x - ox;
                sentinel = std::all_of(err.begin(), err.end(), [tol=threshold](auto error){ return error < tol; });
            }
            k++;
        }
        return k;
    }

    template<typename A, typename X, typename B>
    size_t gradient_descent(const A& a, X& x, const B b, double threshold, size_t maxIterations = 100) {
        X xi = x;

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

        template<typename Matrix>
        Matrix operator()(const Matrix& matrix) const  {
            return Matrix(matrix.rows()).identity();
        }
    };

    template<
        typename A,
        typename X,
        typename B,
        typename Preconditioner>
    size_t conjugate_gradient(const A& a, X& x, const B b,  Preconditioner&& preconditioner, double threshold, size_t maxIterations = 0) {
        maxIterations = maxIterations == 0 ? x.size() : maxIterations;
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

}