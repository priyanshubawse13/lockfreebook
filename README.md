# LockFreeBook — High-Performance Lock-Free Order Book (C++20)

A high-performance bid/ask order book written in modern **C++20**, designed
to demonstrate the lock-free data structures and multi-threaded patterns used
in low-latency trading systems.

## Key Design Decisions

| Decision | Why |
|---|---|
| `std::atomic` + CAS on hot path | Eliminates mutex contention; never blocks |
| Price-level map (sorted) | O(log n) insert/cancel, O(1) BBO query |
| `std::jthread` producer/consumer | Safe, structured concurrency |
| Nanosecond benchmarking harness | Performance is measurable, not assumed |
| Thread sanitizer + GoogleTest | Correctness validated under concurrent stress |

## Features

- **Order operations:** Add, Cancel, Modify — bid and ask sides
- **Lock-free BBO:** Best bid / best offer queries in O(1)
- **Multi-threaded simulation:** producer feeds FIX-style events; consumers process concurrently
- **Benchmarking:** measures latency (ns/op) and throughput (orders/sec)
- **Modern C++20:** `std::atomic`, `std::jthread`, `std::span`, concepts, structured bindings
- **CI:** GitHub Actions — build, tests, sanitizers on every push

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/lockfreebook_bench   # run benchmarks
./build/lockfreebook_test    # run tests
```

## Project Structure

```
include/
  order_book.hpp       # Order book interface
  order.hpp            # Order / price level types
src/
  order_book.cpp       # Lock-free implementation
  main.cpp             # Multi-threaded simulation entry point
bench/
  bench.cpp            # Latency & throughput harness
tests/
  test_order_book.cpp  # GoogleTest unit + stress tests
CMakeLists.txt
.github/workflows/ci.yml
```

## Concepts Demonstrated

- Lock-free programming with `std::atomic<>` and compare-exchange
- Price-level aggregation and order book depth management
- Multi-producer / multi-consumer simulation with `std::jthread`
- Performance measurement at nanosecond granularity
- Thread sanitizer validation, structured CI/CD
