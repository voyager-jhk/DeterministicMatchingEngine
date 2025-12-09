# Deterministic Matching Engine

![alt text](https://img.shields.io/badge/License-MIT-yellow.svg)

![alt text](https://img.shields.io/badge/C%2B%2B-17-00599C.svg?logo=c%2B%2B)

![alt text](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake)

> **A high-performance, deterministic Limit Order Book (LOB) implementation in C++17.**
> Designed with a focus on memory safety, formal verification via invariants, and event-sourced architecture.

---

## 📖 Overview

This project implements a single-threaded, lock-free matching engine optimized for low-latency trading simulations. Unlike traditional matching engines, it employs **Event Sourcing** as a first-class citizen, ensuring that the entire state of the order book can be deterministically reconstructed from a sequence of input events.

Key engineering goals:

- **Correctness**: Verified via property-based testing and rigorous invariant checks.
- **Determinism**: Identical inputs guarantee identical state, facilitating debugging and replay.
- **Performance**: Optimized for cache locality and minimal allocation on the hot path.

---

## ✨ Key Features

- **Compile-Time Safety**: Extensive use of StrongType wrappers to prevent primitive obsession (e.g., confusing Price with Quantity).
- **Event Sourcing Architecture**: All state mutations are derived from an immutable stream of events (NewOrder, Cancel, Trade).
- **Invariant Verification**: Critical business logic (e.g., best_bid < best_ask, volume conservation) is asserted continuously.
- **Deterministic Replay**: Built-in engine to reconstruct historical states efficiently from event logs.
- **Property-Based Testing**: Integrated QuickCheck-style tests to explore edge cases beyond standard unit tests.

---

## ⚡ Performance Benchmarks

**Test Environment**:

- Build: Release mode with `-O3 -march=native`
- Measured: December 2024
- Hardware: 13th Gen Intel(R) Core(TM) i9-13980HX

**Results**:

| Metric         | Value             |
| -------------- | ----------------- |
| **Throughput** | **5M orders/sec** |
| P50 Latency    | 61 ns             |
| P90 Latency    | 77 ns             |
| P99 Latency    | 121 ns            |
| P99.9 Latency  | 255 ns            |
| Max Latency    | 3.5 μs            |

**Stress Test**:

- ✅ 1,000,000 orders processed in 1 second
- ✅ All 6 invariants maintained throughout
- ✅ Memory usage: O(N) linear growth (~625 KB for 10K orders)

**Scenario Breakdown**:

- All orders match: 6.25M ops/sec
- All orders rest: 12.5M ops/sec
- Mixed (50/50): 12.5M ops/sec

**Note**: Performance varies by hardware. These numbers demonstrate the
engine's capability to handle production-level throughput with sub-microsecond
latency on modern hardware.

---

## 🏗️ Architecture

### Core Components

- **OrderBook**: The central entity managing LimitLevel structures (RB-Tree based std::map for price levels).
- **LimitLevel**: Represents a price node containing a FIFO queue of orders.
- **EventSystem**: capturing all external inputs (Orders/Cancels) and internal outputs (Trades).

### Data Structures Rationale

**std::map vs std::unordered_map**:

- std::map (Red-Black Tree) was chosen to maintain price ordering.
- Market data dissemination often requires traversing the top `NN` levels, which is `O(N)O(N)` in a sorted map vs `O(Nlog⁡N)O(NlogN)` sorting a hash map.
- Access to Best Bid/Ask is `O(1)O(1)` via begin()/rbegin().

---

## 🚀 Getting Started

### Prerequisites

- C++17 compliant compiler (GCC/Clang/MSVC)
- CMake 3.10+

### Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Run Examples

```bash
# Run the core demo
./matching_engine_demo

# Run the benchmark suite
./matching_engine_benchmarks
```

---

## 🧪 Testing Strategy

The project employs a multi-layered testing approach:

1. **Unit Tests**: Cover standard trading scenarios (Fill, Partial Fill, Sweep, Cancel).
2. **Property-Based Tests**:Generates random sequences of thousands of orders.Verifies global invariants (e.g., "The book never crosses").Validates replay idempotency (`State(Events)==State(Replay(Events))State(Events)==State(Replay(Events))`).
3. **Sanitizers**: Compatible with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

To run the test suite:

```bash
./matching_engine_unit_tests
./matching_engine_property_tests
```

---

## 🔮 Future Roadmap

- [ ] Implementation of Iceberg and Stop-Loss orders.
- [ ] Snapshotting mechanism for faster recovery.
- [ ] FIX Protocol gateway integration.
- [ ] Lock-free ring buffer for multi-threaded input processing.

---

## 🤝 License

Distributed under the MIT License. See LICENSE for more information.
