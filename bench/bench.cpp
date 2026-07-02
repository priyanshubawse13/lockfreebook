#include "order_book.hpp"
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

static constexpr int WARMUP  = 10'000;
static constexpr int SAMPLES = 100'000;

int main() {
    OrderBook book;

    // Warm up
    for (int i = 0; i < WARMUP; ++i) {
        book.add_order({static_cast<uint64_t>(i), Side::BID, 100.0 + (i % 10), 10});
    }

    // Latency: measure individual add_order calls
    std::vector<long long> latencies(SAMPLES);
    for (int i = 0; i < SAMPLES; ++i) {
        Order o{static_cast<uint64_t>(WARMUP + i), Side::BID,
                100.0 + (i % 20) * 0.01, static_cast<uint32_t>(i % 100 + 1)};
        auto t0 = std::chrono::high_resolution_clock::now();
        book.add_order(o);
        auto t1 = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    }

    std::sort(latencies.begin(), latencies.end());
    double mean = static_cast<double>(
        std::accumulate(latencies.begin(), latencies.end(), 0LL)) / SAMPLES;

    std::cout << "=== LockFreeBook Benchmark ===\n";
    std::cout << "Samples  : " << SAMPLES << "\n";
    std::cout << "Mean     : " << mean            << " ns\n";
    std::cout << "p50      : " << latencies[SAMPLES * 50  / 100] << " ns\n";
    std::cout << "p95      : " << latencies[SAMPLES * 95  / 100] << " ns\n";
    std::cout << "p99      : " << latencies[SAMPLES * 99  / 100] << " ns\n";
    std::cout << "Max      : " << latencies.back()               << " ns\n";

    // Throughput
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < SAMPLES; ++i)
        book.add_order({static_cast<uint64_t>(WARMUP + SAMPLES + i),
                        Side::ASK, 101.0 + (i % 20) * 0.01,
                        static_cast<uint32_t>(i % 50 + 1)});
    auto t1 = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Throughput: " << static_cast<int>(SAMPLES / secs) << " orders/sec\n";
    return 0;
}
