#ifndef REPLAY_HPP
#define REPLAY_HPP

#include "orderbook.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// REPLAY ENGINE - Determinism Verification
// ============================================================================

class ReplayEngine {
public:
    // Replay from in-memory event log
    // Returns a reconstructed OrderBook state
    static OrderBook replay_from_log(const std::vector<Event>& log) {
        // Estimate capacity from log size to avoid reallocation
        OrderBook book(log.size() * 2); 
        
        for (const auto& event : log) {
            std::visit([&book](auto&& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, NewOrderEvent>) {
                    // Replay NewOrder: Re-inject into book
                    book.process_new_order(e.order_id, e.side, e.price, e.quantity);
                }
                else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
                    // Replay Cancel: Re-inject into book
                    book.process_cancel(e.order_id);
                }
            }, event);
        }
        
        return book;
    }
    
    // Save event log to CSV file
    static void save_log(const std::vector<Event>& log, const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        char buffer[256];
        for (const auto& event : log) {
            // Use our zero-alloc to_buffer helper
            event_to_buffer(event, buffer, sizeof(buffer));
            file << buffer << "\n";
        }
    }
    
    // Load events from CSV file
    static std::vector<Event> load_log(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        std::vector<Event> log;
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            // Minimalistic CSV parser (Fast)
            std::stringstream ss(line);
            std::string segment;
            std::vector<std::string> parts;
            
            while (std::getline(ss, segment, ',')) {
                parts.push_back(segment);
            }
            
            if (parts.empty()) continue;
            
            const std::string& type = parts[0];
            
            if (type == "NEW_ORDER" && parts.size() >= 6) {
                // Format: NEW_ORDER,timestamp,id,side,price,qty
                Timestamp ts(std::stoull(parts[1]));
                OrderId id(std::stoull(parts[2]));
                Side side = (parts[3] == "BUY") ? Side::BUY : Side::SELL;
                Price price(std::stoll(parts[4]));
                Quantity qty(std::stoull(parts[5]));
                
                log.emplace_back(std::in_place_type<NewOrderEvent>, ts, id, side, price, qty);
            }
            else if (type == "CANCEL_ORDER" && parts.size() >= 3) {
                // Format: CANCEL_ORDER,timestamp,id
                Timestamp ts(std::stoull(parts[1]));
                OrderId id(std::stoull(parts[2]));
                
                log.emplace_back(std::in_place_type<CancelOrderEvent>, ts, id);
            }
        }
        
        return log;
    }
};

#endif