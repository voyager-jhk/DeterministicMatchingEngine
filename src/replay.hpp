#ifndef REPLAY_HPP
#define REPLAY_HPP

#include "orderbook.hpp"
#include <fstream>
#include <memory>

// ============================================================================
// REPLAY ENGINE - Determinism Verification
// ============================================================================

class ReplayEngine {
public:
    // Replay from event log
    static OrderBook replay_from_log(const std::vector<std::shared_ptr<Event>>& log) {
        OrderBook book;
        
        for (const auto& event : log) {
            if (auto new_order = std::dynamic_pointer_cast<NewOrderEvent>(event)) {
                book.process_new_order(
                    new_order->order_id, 
                    new_order->side, 
                    new_order->price, 
                    new_order->quantity
                );
            } 
            else if (auto cancel = std::dynamic_pointer_cast<CancelOrderEvent>(event)) {
                book.process_cancel(cancel->order_id);
            }
            // Trade events are generated, not replayed
        }
        
        return book;
    }
    
    // Save event log to file
    static void save_log(const std::vector<std::shared_ptr<Event>>& log, 
                        const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        for (const auto& event : log) {
            file << event->to_string() << "\n";
        }
    }
    
    // Load and replay from file
    static OrderBook load_and_replay(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        OrderBook book;
        std::string line;
        
        while (std::getline(file, line)) {
            // Parse event from line
            // Simplified - production would use proper CSV parser
            auto event = parse_event(line);
            
            if (auto new_order = std::dynamic_pointer_cast<NewOrderEvent>(event)) {
                book.process_new_order(
                    new_order->order_id,
                    new_order->side,
                    new_order->price,
                    new_order->quantity
                );
            }
            else if (auto cancel = std::dynamic_pointer_cast<CancelOrderEvent>(event)) {
                book.process_cancel(cancel->order_id);
            }
        }
        
        return book;
    }
    
private:
    // Parse event from CSV line (simplified)
    static std::shared_ptr<Event> parse_event(const std::string& line) {
        // TODO: Implement proper CSV parsing
        // This is a placeholder
        return nullptr;
    }
};

#endif // REPLAY_HPP