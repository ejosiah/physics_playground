#pragma once

#include <benchmark/benchmark.h>
#include <thread>
#include <barrier>
#include <latch>
#include <atomic>
#include <mutex>
#include <thread_pool/thread_pool.hpp>

struct BarrierCollab {
    int64_t numThreads;
    std::barrier<> barrier;
    std::latch start_latch;
    std::vector<std::thread> workers{};


    BarrierCollab(int64_t numThreads)
    : numThreads(numThreads)
    , barrier(numThreads + 1)
    , start_latch(numThreads)
    {}

    ~BarrierCollab() {
        for(auto& worker : workers){
            worker.join();
        }
    }

    std::thread workerThread() {
        return std::thread([this]{
           barrier.arrive_and_wait();
        });
    }

    void run() {
        for(auto i = 0; i < numThreads; i++){
            workers.push_back(std::move(workerThread()));
        }
        barrier.arrive_and_wait();
    }
    
    static void profile(benchmark::State& state) {
        BarrierCollab collab{state.range(0)};

        for(auto _ : state){
            collab.run();
        }
    }
};

struct AtmoicCollabYield {
    int64_t numThreads;
    std::atomic_int done{0};
    std::vector<std::thread> workers{};


    AtmoicCollabYield(int64_t numThreads)
    : numThreads(numThreads)
    {}

    ~AtmoicCollabYield() {
        for(auto& worker : workers){
            worker.join();
        }
    }

    std::thread workerThread() {
        return std::thread([this]{
            done++;
        });
    }

    void run() {
        for(auto i = 0; i < numThreads; i++){
            workers.push_back(std::move(workerThread()));
        }
        while(done.load() < numThreads){
            std::this_thread::yield();
        }
    }

    static void profile(benchmark::State& state) {
        AtmoicCollabYield collab{state.range(0)};

        for(auto _ : state){
            collab.run();
        }
    }
};

struct AtmoicCollabBusySpin {
    int64_t numThreads;
    std::atomic_int done{0};
    std::vector<std::thread> workers{};

    AtmoicCollabBusySpin(int64_t numThreads)
            : numThreads(numThreads)
    {}

    ~AtmoicCollabBusySpin() {
        for(auto& worker : workers){
            worker.join();
        }
    }

    std::thread workerThread() {
        return std::thread([this]{
            done++;
        });
    }

    void run() {
        for(auto i = 0; i < numThreads; i++){
            workers.push_back(std::move(workerThread()));
        }
        while(done.load() < numThreads){

        }
    }

    static void profile(benchmark::State& state) {
        AtmoicCollabBusySpin collab{state.range(0)};

        for(auto _ : state){
            collab.run();
        }
    }
};

struct MutexCollab {
    int64_t numThreads;
    std::mutex mutex;
    int done{0};
    std::vector<std::thread> workers{};


    MutexCollab(int64_t numThreads)
    : numThreads(numThreads)
            {}

    ~MutexCollab() {
        for(auto& worker : workers){
            worker.join();
        }
    }

    std::thread workerThread() {
        return std::thread([this]{
            std::unique_lock<std::mutex> lk(mutex);
            done++;
        });
    }

    void run() {
        for(auto i = 0; i < numThreads; i++){
            workers.push_back(std::move(workerThread()));
        }
        while(true){
            std::unique_lock<std::mutex> lk(mutex);
            if(done >= numThreads) break;
            lk.unlock();
        }
    }

    static void profile(benchmark::State& state) {
        MutexCollab collab{state.range(0)};

        for(auto _ : state){
            collab.run();
        }
    }
    
};

struct ThreadPoolCollab {
    tp::ThreadPool threadPool;

    ThreadPoolCollab(int64_t numThreads)
    : threadPool(numThreads)
    {}
    
    void run() {
        for(auto i = 0; i < threadPool.m_thread_count; i++){
            threadPool.addTask([]{});
        }
        threadPool.waitForCompletion();
    }

    static void profile(benchmark::State& state) {
        ThreadPoolCollab collab{state.range(0)};

        for(auto _ : state){
            collab.run();
        }
    }
};

BENCHMARK(BarrierCollab::profile)->RangeMultiplier(2)->Range(1, 1 << 5);
//BENCHMARK(AtmoicCollabYield::profile)->RangeMultiplier(2)->Range(1, 1 << 5);
//BENCHMARK(AtmoicCollabBusySpin::profile)->RangeMultiplier(2)->Range(1, 1 << 5);
//BENCHMARK(MutexCollab::profile)->RangeMultiplier(2)->Range(1, 1 << 5);
//BENCHMARK(ThreadPoolCollab::profile)->RangeMultiplier(2)->Range(1, 1 << 5);