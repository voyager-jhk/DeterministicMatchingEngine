#include "orderbook.hpp"
#include "replay.hpp"
#include <iostream>

// ============================================================================
// MAIN - Interactive Demonstration
// ============================================================================

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║   DETERMINISTIC MATCHING ENGINE - Jane Street Project   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    OrderBook book;
    
    std::cout << "========== SCENARIO 1: Building Order Book ==========\n";
    std::cout << "\n📝 Adding sell orders...\n";
    book.process_new_order(OrderId(1), Side::SELL, Price(101.0), Quantity(50));
    book.process_new_order(OrderId(2), Side::SELL, Price(100.5), Quantity(30));
    book.process_new_order(OrderId(3), Side::SELL, Price(100.0), Quantity(20));
    book.print_book();
    
    std::cout << "\n📝 Adding buy orders...\n";
    book.process_new_order(OrderId(4), Side::BUY, Price(99.0), Quantity(40));
    book.process_new_order(OrderId(5), Side::BUY, Price(99.5), Quantity(35));
    book.print_book();
    
    std::cout << "\n========== SCENARIO 2: Aggressive Order ==========\n";
    std::cout << "\n💥 Aggressive buy order (sweeps multiple levels)...\n";
    book.process_new_order(OrderId(6), Side::BUY, Price(101.5), Quantity(80));
    book.print_book();
    
    std::cout << "\n========== SCENARIO 3: Order Cancellation ==========\n";
    std::cout << "\n🗑️  Cancelling order ID 4...\n";
    book.process_cancel(OrderId(4));
    book.print_book();
    
    std::cout << "\n========== SCENARIO 4: Market Order ==========\n";
    std::cout << "\n📊 Market buy order (price = infinity)...\n";
    book.process_new_order(OrderId(7), Side::BUY, Price(999999.0), Quantity(25));
    book.print_book();
    
    std::cout << "\n========== EVENT LOG ==========\n";
    const auto& events = book.get_event_log();
    std::cout << "Total events: " << events.size() << "\n\n";
    
    std::cout << "Recent events:\n";
    for (size_t i = events.size() >= 5 ? events.size() - 5 : 0; 
         i < events.size(); ++i) {
        std::cout << "  " << events[i]->to_string() << "\n";
    }
    
    std::cout << "\n========== DETERMINISTIC REPLAY ==========\n";
    std::cout << "\n💾 Saving event log...\n";
    ReplayEngine::save_log(events, "matching_engine.log");
    std::cout << "✓ Saved to matching_engine.log\n";
    
    std::cout << "\n🔄 Replaying from log...\n";
    OrderBook replayed = ReplayEngine::replay_from_log(events);
    
    std::cout << "\nOriginal book:\n";
    book.print_book();
    
    std::cout << "\nReplayed book:\n";
    replayed.print_book();
    
    // Verify they're identical
    bool identical = true;
    auto orig_bid = book.best_bid();
    auto repl_bid = replayed.best_bid();
    auto orig_ask = book.best_ask();
    auto repl_ask = replayed.best_ask();
    
    if (orig_bid.has_value() != repl_bid.has_value() ||
        (orig_bid.has_value() && orig_bid->get() != repl_bid->get())) {
        identical = false;
    }
    
    if (orig_ask.has_value() != repl_ask.has_value() ||
        (orig_ask.has_value() && orig_ask->get() != repl_ask->get())) {
        identical = false;
    }
    
    std::cout << "\n" << (identical ? "✅" : "❌") 
              << " Replay verification: " 
              << (identical ? "PASSED" : "FAILED") << "\n";
    
    std::cout << "\n========== INVARIANT CHECKS ==========\n";
    std::cout << (book.check_invariants() ? "✅" : "❌") 
              << " All invariants satisfied\n";
    
    std::cout << "\n✨ Demonstration complete!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "  ✓ Price-time priority (FIFO)\n";
    std::cout << "  ✓ Multi-level order sweeping\n";
    std::cout << "  ✓ Order cancellation\n";
    std::cout << "  ✓ Market orders\n";
    std::cout << "  ✓ Event sourcing and replay\n";
    std::cout << "  ✓ Invariant verification\n";
    std::cout << "  ✓ Complete determinism\n";
    
    return 0;
}