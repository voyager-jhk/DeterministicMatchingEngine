#include "../src/orderbook.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

class Benchmark {
public:
    static void run_all_benchmarks() {
        std::cout << "\n========== PERFORMANCE BENCHMARKS ==========\n\n";
        
        benchmark_throughput();
        benchmark_latency();
        benchmark_memory();
        benchmark_cancel();
    }
    
private:
    static void benchmark_throughput() {
        std::cout << "Benchmark 1: Throughput Test\n";
        OrderBook book;
        const int num_orders = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_orders; ++i) {
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            double price = 100.0 + (i % 10) * 0.1;
            book.process_new_order(OrderId(i), side, Price(price), Quantity(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double throughput = (num_orders * 1000000.0) / duration.count();
        std::cout << "  Processed: " << num_orders << " orders\n";
        std::cout << "  Time: " << duration.count() << " μs\n";
        std::cout << "  Throughput: " << static_cast<int>(throughput) << " orders/sec\n";
        std::cout << "  Avg latency: " << duration.count() / num_orders << " μs/order\n\n";
    }
    
    static void benchmark_latency() {
        std::cout << "Benchmark 2: Latency Distribution\n";
        OrderBook book;
        std::vector<long long> latencies;
        
        // Pre-populate book
        for (int i = 0; i < 1000; ++i) {
            book.process_new_order(OrderId(i), Side::SELL, 
                                  Price(100.0 + i*0.1), Quantity(10));
        }
        
        // Measure latency of individual orders
        for (int i = 0; i < 10000; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            book.process_new_order(OrderId(1000+i), Side::BUY, 
                                  Price(105.0), Quantity(10));
            auto end = std::chrono::high_resolution_clock::now();
            
            latencies.push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
            );
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        std::cout << "  Samples: " << latencies.size() << "\n";
        std::cout << "  P50: " << latencies[latencies.size()/2] << " ns\n";
        std::cout << "  P90: " << latencies[latencies.size()*9/10] << " ns\n";
        std::cout << "  P99: " << latencies[latencies.size()*99/100] << " ns\n";
        std::cout << "  P99.9: " << latencies[latencies.size()*999/1000] << " ns\n";
        std::cout << "  Max: " << latencies.back() << " ns\n\n";
    }
    
    static void benchmark_memory() {
        std::cout << "Benchmark 3: Memory Usage\n";
        OrderBook book;
        
        const int num_orders = 10000;
        size_t initial_size = sizeof(book);
        
        for (int i = 0; i < num_orders; ++i) {
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            double price = 100.0 + (i % 100) * 0.1;
            book.process_new_order(OrderId(i), side, Price(price), Quantity(10));
        }
        
        // Estimate memory (rough approximation)
        size_t estimated_per_order = sizeof(Order) + sizeof(std::shared_ptr<Order>);
        size_t estimated_total = initial_size + num_orders * estimated_per_order;
        
        std::cout << "  Orders in book: " << num_orders << "\n";
        std::cout << "  Estimated memory per order: " << estimated_per_order << " bytes\n";
        std::cout << "  Estimated total memory: " << estimated_total / 1024 << " KB\n";
        std::cout << "  Events logged: " << book.get_event_log().size() << "\n\n";
    }
    
    static void benchmark_cancel() {
        std::cout << "Benchmark 4: Cancel Performance\n";
        OrderBook book;
        std::vector<OrderId> order_ids;
        
        // Add many orders
        const int num_orders = 10000;
        for (int i = 0; i < num_orders; ++i) {
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            double price = 100.0 + (i % 100) * 0.1;
            book.process_new_order(OrderId(i), side, Price(price), Quantity(10));
            order_ids.push_back(OrderId(i));
        }
        
        // Benchmark cancellations
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            book.process_cancel(order_ids[i]);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "  Cancelled: 1000 orders\n";
        std::cout << "  Time: " << duration.count() << " μs\n";
        std::cout << "  Avg per cancel: " << duration.count() / 1000.0 << " μs\n";
        std::cout << "  Note: O(N) complexity - can be optimized to O(1)\n\n";
    }
};

// ============================================================================
// STRESS TEST
// ============================================================================

class StressTest {
public:
    static void run_stress_test() {
        std::cout << "\n========== STRESS TEST ==========\n\n";
        
        std::cout << "Running 1 million order test...\n";
        OrderBook book;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000000; ++i) {
            Side side = (i % 3 == 0) ? Side::BUY : Side::SELL;
            double price = 100.0 + (i % 50) * 0.01;
            book.process_new_order(OrderId(i), side, Price(price), Quantity(i % 100 + 1));
            
            // Periodic cancellations
            if (i % 100 == 0 && i > 0) {
                book.process_cancel(OrderId(i - 50));
            }
            
            // Periodic invariant checks
            if (i % 10000 == 0) {
                if (!book.check_invariants()) {
                    std::cerr << "Invariant violation at order " << i << "\n";
                    throw std::runtime_error("Stress test failed");
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        
        std::cout << "  ✓ Processed 1,000,000 orders\n";
        std::cout << "  Time: " << duration.count() << " seconds\n";
        std::cout << "  Throughput: " << 1000000 / duration.count() << " orders/sec\n";
        std::cout << "  All invariants maintained throughout\n";
    }
};

// ============================================================================
// COMPARISON TEST
// ============================================================================

class ComparisonTest {
public:
    static void compare_scenarios() {
        std::cout << "\n========== SCENARIO COMPARISON ==========\n\n";
        
        std::cout << "Scenario 1: All orders match immediately\n";
        benchmark_scenario_all_match();
        
        std::cout << "\nScenario 2: All orders rest on book\n";
        benchmark_scenario_all_rest();
        
        std::cout << "\nScenario 3: Mixed (50% match, 50% rest)\n";
        benchmark_scenario_mixed();
    }
    
private:
    static void benchmark_scenario_all_match() {
        OrderBook book;
        const int num_pairs = 50000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_pairs; ++i) {
            book.process_new_order(OrderId(i*2), Side::SELL, Price(100.0), Quantity(10));
            book.process_new_order(OrderId(i*2+1), Side::BUY, Price(100.0), Quantity(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "  Throughput: " << (num_pairs * 2 * 1000) / duration.count() 
                  << " orders/sec\n";
    }
    
    static void benchmark_scenario_all_rest() {
        OrderBook book;
        const int num_orders = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_orders; ++i) {
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            double price = side == Side::BUY ? 99.0 : 101.0;
            book.process_new_order(OrderId(i), side, Price(price), Quantity(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "  Throughput: " << (num_orders * 1000) / duration.count() 
                  << " orders/sec\n";
    }
    
    static void benchmark_scenario_mixed() {
        OrderBook book;
        const int num_orders = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_orders; ++i) {
            Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            double price = (i % 4 < 2) ? 100.0 : (side == Side::BUY ? 99.0 : 101.0);
            book.process_new_order(OrderId(i), side, Price(price), Quantity(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "  Throughput: " << (num_orders * 1000) / duration.count() 
                  << " orders/sec\n";
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           PERFORMANCE BENCHMARK SUITE                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    try {
        Benchmark::run_all_benchmarks();
        ComparisonTest::compare_scenarios();
        StressTest::run_stress_test();
        
        std::cout << "\n✅ All benchmarks completed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed with exception: " << e.what() << "\n";
        return 1;
    }
}