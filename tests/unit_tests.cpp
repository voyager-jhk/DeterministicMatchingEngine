#include "../src/orderbook.hpp"
#include "../src/replay.hpp"
#include <iostream>
#include <cassert>
#include <variant>
#include <vector>

// ============================================================================
// CUSTOM ASSERTION MACRO (Works in Release Mode)
// ============================================================================
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "❌ Assertion failed: " << #cond \
                      << "\n   File: " << __FILE__ \
                      << "\n   Line: " << __LINE__ << std::endl; \
            throw std::runtime_error("Test assertion failed"); \
        } \
    } while(0)

// ============================================================================
// UNIT TEST SUITE
// ============================================================================

class TestSuite {
public:
    static void run_all_tests() {
        std::cout << "Running Matching Engine Test Suite...\n\n";
        
        try {
            test_simple_fill();
            test_partial_fill();
            test_multi_level_sweep();
            test_cancel_order();
            test_price_time_priority();
            test_invariants();
            test_replay_determinism();
            test_empty_book();
            test_crossed_order();
            
            std::cout << "\n✅ All tests passed!\n";
        } catch (const std::exception& e) {
            std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
            exit(1);
        }
    }
    
private:
    // Helper: Compare internal price with expected double (approximate match not needed for integers)
    static bool eq_price(Price p, double expected) {
        return p.get() == static_cast<int64_t>(expected * PRICE_SCALE);
    }

    static void test_simple_fill() {
        std::cout << "Test 1: Simple Fill... ";
        OrderBook book;
        
        // Sell 10 @ 100.0
        book.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        // Buy 10 @ 100.0 -> Match
        book.process_new_order(OrderId(2), Side::BUY, from_double(100.0), Quantity(10));
        
        TEST_ASSERT(!book.best_bid().has_value());
        TEST_ASSERT(!book.best_ask().has_value());
        std::cout << "Passed\n";
    }
    
    static void test_partial_fill() {
        std::cout << "Test 2: Partial Fill... ";
        OrderBook book;
        
        // Sell 10 @ 100.0
        book.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        // Buy 5 @ 100.0 -> Partial Match
        book.process_new_order(OrderId(2), Side::BUY, from_double(100.0), Quantity(5));
        
        // Remaining Sell 5 @ 100.0
        TEST_ASSERT(book.best_ask().has_value());
        TEST_ASSERT(eq_price(*book.best_ask(), 100.0));
        
        // Check event log for Trade
        const auto& log = book.get_event_log();
        TEST_ASSERT(!log.empty());
        bool found_trade = false;
        for(const auto& evt : log) {
            if (std::holds_alternative<TradeEvent>(evt)) {
                const auto& trade = std::get<TradeEvent>(evt);
                if (trade.quantity.get() == 5) found_trade = true;
            }
        }
        TEST_ASSERT(found_trade);
        std::cout << "Passed\n";
    }
    
    static void test_multi_level_sweep() {
        std::cout << "Test 3: Multi-Level Sweep... ";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, from_double(101.0), Quantity(10));
        book.process_new_order(OrderId(3), Side::SELL, from_double(102.0), Quantity(10));
        
        // Buy 25 @ 105.0 -> Sweeps 100.0 and 101.0, eats 5 of 102.0
        book.process_new_order(OrderId(4), Side::BUY, from_double(105.0), Quantity(25));
        
        TEST_ASSERT(book.best_ask().has_value());
        TEST_ASSERT(eq_price(*book.best_ask(), 102.0));
        std::cout << "Passed\n";
    }
    
    static void test_cancel_order() {
        std::cout << "Test 4: Cancel Order... ";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        book.process_cancel(OrderId(1));
        
        TEST_ASSERT(!book.best_ask().has_value());
        
        // Verify cancel event
        const auto& log = book.get_event_log();
        TEST_ASSERT(std::holds_alternative<CancelOrderEvent>(log.back()));
        std::cout << "Passed\n";
    }
    
    static void test_price_time_priority() {
        std::cout << "Test 5: Price-Time Priority... ";
        OrderBook book;
        
        // Sell orders at same price
        book.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, from_double(100.0), Quantity(10));
        
        // Buy 5 -> Should match with Order 1 (First in)
        book.process_new_order(OrderId(3), Side::BUY, from_double(100.0), Quantity(5));
        
        const auto& log = book.get_event_log();
        TEST_ASSERT(!log.empty());
        
        // Verify last event is Trade matching Order 1
        const Event& last_evt = log.back();
        const TradeEvent* trade = std::get_if<TradeEvent>(&last_evt);
        
        TEST_ASSERT(trade != nullptr);
        TEST_ASSERT(trade->passive_order_id.get() == 1);
        TEST_ASSERT(trade->aggressive_order_id.get() == 3);
        std::cout << "Passed\n";
    }
    
    static void test_invariants() {
        std::cout << "Test 6: Invariants... ";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::BUY, from_double(99.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, from_double(101.0), Quantity(10));
        
        // Manual invariant check: Bid < Ask
        TEST_ASSERT(book.best_bid().has_value());
        TEST_ASSERT(book.best_ask().has_value());
        TEST_ASSERT(book.best_bid()->get() < book.best_ask()->get());
        std::cout << "Passed\n";
    }
    
    static void test_replay_determinism() {
        std::cout << "Test 7: Replay Determinism... ";
        OrderBook book1;
        
        book1.process_new_order(OrderId(1), Side::SELL, from_double(100.0), Quantity(10));
        book1.process_new_order(OrderId(2), Side::BUY, from_double(100.0), Quantity(5)); // Trade
        book1.process_new_order(OrderId(3), Side::SELL, from_double(101.0), Quantity(10));
        
        auto log = book1.get_event_log();
        
        // Replay
        OrderBook book2 = ReplayEngine::replay_from_log(log);
        
        // Verify state equality
        // 1. Check Best Ask
        TEST_ASSERT(book1.best_ask().has_value() == book2.best_ask().has_value());
        if (book1.best_ask().has_value()) {
            TEST_ASSERT(book1.best_ask()->get() == book2.best_ask()->get());
        }
        
        // 2. Check Best Bid
        TEST_ASSERT(book1.best_bid().has_value() == book2.best_bid().has_value());
        
        std::cout << "Passed\n";
    }
    
    static void test_empty_book() {
        std::cout << "Test 8: Empty Book Edge Case... ";
        OrderBook book;
        
        TEST_ASSERT(!book.best_bid().has_value());
        TEST_ASSERT(!book.best_ask().has_value());
        
        book.process_cancel(OrderId(999)); // Should not crash
        std::cout << "Passed\n";
    }
    
    static void test_crossed_order() {
        std::cout << "Test 9: Crossed Order Prevention... ";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::BUY, from_double(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, from_double(101.0), Quantity(10));
        
        // Aggressive Buy @ 102.0 (Crosses 101.0) -> Match
        book.process_new_order(OrderId(3), Side::BUY, from_double(102.0), Quantity(10));
        
        // After match, book should be uncrossed
        if (book.best_bid().has_value() && book.best_ask().has_value()) {
            TEST_ASSERT(book.best_bid()->get() < book.best_ask()->get());
        }
        std::cout << "Passed\n";
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              UNIT TEST SUITE                               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    try {
        TestSuite::run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << "\n";
        return 1;
    }
}