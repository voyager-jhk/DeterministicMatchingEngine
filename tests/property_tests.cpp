#include "../src/orderbook.hpp"
#include "../src/replay.hpp"
#include <iostream>
#include <random>
#include <cassert>

// ============================================================================
// PROPERTY-BASED TESTING (QuickCheck style)
// ============================================================================

class PropertyTesting {
private:
    std::mt19937 rng{std::random_device{}()};
    
    struct RandomOrder {
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;
    };
    
    RandomOrder generate_random_order(uint64_t id) {
        std::uniform_int_distribution<> side_dist(0, 1);
        std::uniform_real_distribution<> price_dist(95.0, 105.0);
        std::uniform_int_distribution<uint64_t> qty_dist(1, 1000);
        
        return {
            OrderId(id),
            side_dist(rng) == 0 ? Side::BUY : Side::SELL,
            Price(std::round(price_dist(rng) * 100.0) / 100.0), // 0.01 tick
            Quantity(qty_dist(rng))
        };
    }
    
public:
    // Property 1: 订单簿永不交叉
    void test_never_crosses() {
        std::cout << "\n🔬 Property Test 1: Book Never Crosses\n";
        
        for (int trial = 0; trial < 1000; ++trial) {
            OrderBook book;
            
            // 随机生成 100 个订单
            for (uint64_t i = 0; i < 100; ++i) {
                auto order = generate_random_order(trial * 100 + i);
                book.process_new_order(order.id, order.side, order.price, order.quantity);
                
                // 验证不变量
                if (!book.check_invariants()) {
                    std::cerr << "FAILED at trial " << trial << ", order " << i << "\n";
                    throw std::runtime_error("Invariant violation detected");
                }
            }
        }
        
        std::cout << "  ✓ Passed 1000 trials with 100,000 orders\n";
    }
    
    // Property 2: 重放产生相同结果
    void test_replay_idempotence() {
        std::cout << "\n🔬 Property Test 2: Replay Idempotence\n";
        
        for (int trial = 0; trial < 100; ++trial) {
            OrderBook book1;
            
            // 生成随机订单序列
            for (uint64_t i = 0; i < 50; ++i) {
                auto order = generate_random_order(trial * 50 + i);
                book1.process_new_order(order.id, order.side, order.price, order.quantity);
            }
            
            // 重放
            OrderBook book2 = ReplayEngine::replay_from_log(book1.get_event_log());
            
            // 验证结果完全一致
            auto bid1 = book1.best_bid();
            auto bid2 = book2.best_bid();
            (void)bid2;
            auto ask1 = book1.best_ask();
            auto ask2 = book2.best_ask();
            (void)ask2;
            
            assert(bid1.has_value() == bid2.has_value());
            assert(ask1.has_value() == ask2.has_value());
            
            if (bid1.has_value()) {
                assert(std::abs(bid1->get() - bid2->get()) < 1e-9);
            }
            if (ask1.has_value()) {
                assert(std::abs(ask1->get() - ask2->get()) < 1e-9);
            }
        }
        
        std::cout << "  ✓ Passed 100 replay trials\n";
    }
    
    // Property 3: 成交量守恒
    void test_volume_conservation() {
        std::cout << "\n🔬 Property Test 3: Volume Conservation\n";
        
        for (int trial = 0; trial < 1000; ++trial) {
            OrderBook book;
            
            uint64_t total_buy = 0;
            uint64_t total_sell = 0;
            
            for (uint64_t i = 0; i < 100; ++i) {
                auto order = generate_random_order(trial * 100 + i);
                book.process_new_order(order.id, order.side, order.price, order.quantity);
                
                if (order.side == Side::BUY) {
                    total_buy += order.quantity.get();
                } else {
                    total_sell += order.quantity.get();
                }
            }
            
            // 计算成交量
            uint64_t traded = 0;
            for (const auto& event : book.get_event_log()) {
                if (auto trade = std::dynamic_pointer_cast<TradeEvent>(event)) {
                    traded += trade->quantity.get();
                }
            }
            
            // 验证：总量 >= 成交量
            assert(traded <= std::min(total_buy, total_sell));
        }
        
        std::cout << "  ✓ Volume conservation verified in 1000 trials\n";
    }
    
    // Property 4: FIFO 顺序
    void test_fifo_order() {
        std::cout << "\n🔬 Property Test 4: FIFO Priority\n";
        
        OrderBook book;
        
        // 在同一价格添加多个订单
        for (uint64_t i = 0; i < 10; ++i) {
            book.process_new_order(OrderId(i), Side::SELL, Price(100.0), Quantity(10));
        }
        
        // 大买单扫单
        book.process_new_order(OrderId(100), Side::BUY, Price(100.0), Quantity(100));
        
        // 验证成交顺序
        std::vector<uint64_t> trade_sequence;
        for (const auto& event : book.get_event_log()) {
            if (auto trade = std::dynamic_pointer_cast<TradeEvent>(event)) {
                trade_sequence.push_back(trade->passive_order_id.get());
            }
        }
        
        // 应该严格按 0, 1, 2, ..., 9 的顺序成交
        for (size_t i = 0; i < trade_sequence.size(); ++i) {
            assert(trade_sequence[i] == i);
        }
        
        std::cout << "  ✓ FIFO order maintained\n";
    }
    
    // Property 5: 单调性 - 最优价格随时间变化的合理性
    void test_price_monotonicity() {
        std::cout << "\n🔬 Property Test 5: Price Reasonableness\n";
        
        OrderBook book;
        
        // 添加初始订单簿
        book.process_new_order(OrderId(1), Side::BUY, Price(99.0), Quantity(100));
        book.process_new_order(OrderId(2), Side::SELL, Price(101.0), Quantity(100));
        
        // 随机添加订单，验证价差的合理性
        for (int i = 0; i < 100; ++i) {
            auto order = generate_random_order(10 + i);
            book.process_new_order(order.id, order.side, order.price, order.quantity);
            
            auto bid = book.best_bid();
            auto ask = book.best_ask();
            
            if (bid.has_value() && ask.has_value()) {
                double spread = ask->get() - bid->get();
                (void)spread;
                // 价差必须非负
                assert(spread >= 0);
            }
        }
        
        std::cout << "  ✓ Price spread remains non-negative\n";
    }
    
    void run_all() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "PROPERTY-BASED TEST SUITE\n";
        std::cout << std::string(60, '=') << "\n";
        
        test_never_crosses();
        test_replay_idempotence();
        test_volume_conservation();
        test_fifo_order();
        test_price_monotonicity();
        
        std::cout << "\n✅ All property tests passed!\n";
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           PROPERTY-BASED TEST SUITE                        ║\n";
    std::cout << "║   (QuickCheck-style randomized testing)                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    try {
        PropertyTesting tests;
        tests.run_all();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}