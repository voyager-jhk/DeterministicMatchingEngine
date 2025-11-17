#include "../src/orderbook.hpp"
#include "../src/replay.hpp"
#include <iostream>
#include <cassert>

// ============================================================================
// UNIT TEST SUITE
// ============================================================================

class TestSuite {
public:
    static void run_all_tests() {
        std::cout << "Running Matching Engine Test Suite...\n\n";
        
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
    }
    
private:
    static void test_simple_fill() {
        std::cout << "Test 1: Simple Fill\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::BUY, Price(100.0), Quantity(10));
        
        assert(!book.best_bid().has_value());
        assert(!book.best_ask().has_value());
        std::cout << "  ✓ Orders fully matched\n";
    }
    
    static void test_partial_fill() {
        std::cout << "Test 2: Partial Fill\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::BUY, Price(100.0), Quantity(5));
        
        assert(book.best_ask().has_value());
        assert(book.best_ask().value().get() == 100.0);
        std::cout << "  ✓ Partial fill handled correctly\n";
    }
    
    static void test_multi_level_sweep() {
        std::cout << "Test 3: Multi-Level Sweep\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, Price(101.0), Quantity(10));
        book.process_new_order(OrderId(3), Side::SELL, Price(102.0), Quantity(10));
        book.process_new_order(OrderId(4), Side::BUY, Price(105.0), Quantity(25));
        
        assert(book.best_ask().value().get() == 102.0);
        std::cout << "  ✓ Multi-level sweep executed correctly\n";
    }
    
    static void test_cancel_order() {
        std::cout << "Test 4: Cancel Order\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book.process_cancel(OrderId(1));
        
        assert(!book.best_ask().has_value());
        std::cout << "  ✓ Order cancellation works\n";
    }
    
    static void test_price_time_priority() {
        std::cout << "Test 5: Price-Time Priority\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(3), Side::BUY, Price(100.0), Quantity(5));
        
        // Order 1 should be matched first (FIFO)
        const auto& log = book.get_event_log();
        auto last_trade = std::dynamic_pointer_cast<TradeEvent>(log.back());
        assert(last_trade->passive_order_id.get() == 1);
        std::cout << "  ✓ FIFO priority maintained\n";
    }
    
    static void test_invariants() {
        std::cout << "Test 6: Invariants\n";
        OrderBook book;
        
        book.process_new_order(OrderId(1), Side::BUY, Price(99.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, Price(101.0), Quantity(10));
        
        assert(book.check_invariants());
        assert(book.best_bid().value().get() < book.best_ask().value().get());
        std::cout << "  ✓ All invariants satisfied\n";
    }
    
    static void test_replay_determinism() {
        std::cout << "Test 7: Replay Determinism\n";
        OrderBook book1;
        
        book1.process_new_order(OrderId(1), Side::SELL, Price(100.0), Quantity(10));
        book1.process_new_order(OrderId(2), Side::BUY, Price(100.0), Quantity(5));
        book1.process_new_order(OrderId(3), Side::SELL, Price(101.0), Quantity(10));
        
        auto log = book1.get_event_log();
        OrderBook book2 = ReplayEngine::replay_from_log(log);
        
        assert(book1.best_ask().value().get() == book2.best_ask().value().get());
        std::cout << "  ✓ Replay produces identical state\n";
    }
    
    static void test_empty_book() {
        std::cout << "Test 8: Empty Book Edge Case\n";
        OrderBook book;
        
        assert(!book.best_bid().has_value());
        assert(!book.best_ask().has_value());
        assert(book.check_invariants());
        std::cout << "  ✓ Empty book handled correctly\n";
    }
    
    static void test_crossed_order() {
        std::cout << "Test 9: Crossed Order Prevention\n";
        OrderBook book;
        
        // Add normal orders
        book.process_new_order(OrderId(1), Side::BUY, Price(100.0), Quantity(10));
        book.process_new_order(OrderId(2), Side::SELL, Price(101.0), Quantity(10));
        
        // Verify book is not crossed
        assert(book.best_bid().value().get() < book.best_ask().value().get());
        
        // Try to cross with aggressive order - should match immediately
        book.process_new_order(OrderId(3), Side::BUY, Price(102.0), Quantity(10));
        
        // After matching, book should still not be crossed
        if (book.best_bid().has_value() && book.best_ask().has_value()) {
            assert(book.best_bid().value().get() < book.best_ask().value().get());
        }
        
        std::cout << "  ✓ Book never crosses\n";
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
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}