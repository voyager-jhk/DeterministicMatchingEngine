#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include "types.hpp"
#include "order.hpp"
#include "events.hpp"
#include <map>
#include <vector>
#include <optional>
#include <iostream>
#include <algorithm>
#include <cassert>

// ============================================================================
// ORDER BOOK - Core Matching Engine
// ============================================================================

class OrderBook {
private:
    // Bids: highest first, Asks: lowest first
    std::map<double, LimitLevel, std::greater<double>> bids_;
    std::map<double, LimitLevel, std::less<double>> asks_;
    std::map<uint64_t, std::shared_ptr<Order>> order_map_;
    
    std::vector<std::shared_ptr<Event>> event_log_;
    Timestamp current_time_;

private:
    // 模板辅助函数，可以处理任何类型的 map
    template <typename MapType>
    void remove_from_map(MapType& levels, std::shared_ptr<Order> order) {
        // [这里是你的原始逻辑，原封不动地复制过来]
        auto it = levels.find(order->price.get());
        if (it != levels.end()) {
            std::queue<std::shared_ptr<Order>> temp;
            while (!it->second.orders.empty()) {
                auto o = it->second.orders.front();
                it->second.orders.pop();
                if (o->id.get() != order->id.get()) {
                    temp.push(o);
                }
            }
            it->second.orders = temp;
            it->second.total_volume = Quantity(
                it->second.total_volume.get() - order->remaining_qty.get()
            );

            if (it->second.empty()) {
                levels.erase(it);
            }
        }
    }
    
public:
    OrderBook() : current_time_(Timestamp(0)) {}
    
    // Main entry point for new orders
    void process_new_order(OrderId id, Side side, Price price, Quantity qty) {
        current_time_ = Timestamp(current_time_.get() + 1);
        
        // Log event
        auto event = std::make_shared<NewOrderEvent>(current_time_, id, side, price, qty);
        event_log_.push_back(event);
        
        // Create order
        auto order = std::make_shared<Order>(id, current_time_, side, price, qty);
        order_map_[id.get()] = order;
        
        // Try to match
        if (side == Side::BUY) {
            match_order_buy(order);
        } else {
            match_order_sell(order);
        }
        
        // Add to book if not fully filled
        if (!order->is_filled()) {
            add_to_book(order);
        }
        
        // Verify invariants
        assert(check_invariants());
    }
    
    // Cancel order
    void process_cancel(OrderId id) {
        current_time_ = Timestamp(current_time_.get() + 1);
        
        auto event = std::make_shared<CancelOrderEvent>(current_time_, id);
        event_log_.push_back(event);
        
        auto it = order_map_.find(id.get());
        if (it != order_map_.end()) {
            remove_from_book(it->second);
            order_map_.erase(it);
        }
        
        assert(check_invariants());
    }
    
    // Query best prices
    std::optional<Price> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return Price(bids_.begin()->first);
    }
    
    std::optional<Price> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return Price(asks_.begin()->first);
    }
    
    // Get event log
    const std::vector<std::shared_ptr<Event>>& get_event_log() const {
        return event_log_;
    }
    
    // Print order book
    void print_book(std::ostream& os = std::cout) const {
        os << "\n========== ORDER BOOK ==========\n";
        os << "ASKS:\n";
        for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
            os << "  " << it->first << " : " << it->second.total_volume.get() 
               << " (" << it->second.size() << " orders)\n";
        }
        os << "-----------------------------\n";
        for (const auto& [price, level] : bids_) {
            os << "  " << price << " : " << level.total_volume.get()
               << " (" << level.size() << " orders)\n";
        }
        os << "BIDS\n";
        os << "================================\n";
    }
    
    // Check all invariants
    bool check_invariants() const {
        // 1. Best bid < best ask
        if (!bids_.empty() && !asks_.empty()) {
            if (bids_.begin()->first >= asks_.begin()->first) {
                std::cerr << "INVARIANT VIOLATION: Crossed book!\n";
                return false;
            }
        }
        
        // 2. All levels satisfy their invariants
        for (const auto& [_, level] : bids_) {
            if (!level.check_invariants()) return false;
        }
        for (const auto& [_, level] : asks_) {
            if (!level.check_invariants()) return false;
        }
        
        // 3. All orders in map are valid
        for (const auto& [_, order] : order_map_) {
            if (!order->check_invariants()) return false;
        }
        
        return true;
    }
    
private:
    // Match buy order against asks
    void match_order_buy(std::shared_ptr<Order> order) {
        while (!order->is_filled() && !asks_.empty()) {
            auto& [ask_price, ask_level] = *asks_.begin();
            
            // Check if order crosses
            if (order->price.get() < ask_price) break;
            
            // Match with best ask
            auto passive_order = ask_level.front();
            uint64_t trade_qty = std::min(
                order->remaining_qty.get(), 
                passive_order->remaining_qty.get()
            );
            
            // Generate trade event
            current_time_ = Timestamp(current_time_.get() + 1);
            auto trade = std::make_shared<TradeEvent>(
                current_time_, passive_order->id, order->id,
                Price(ask_price), Quantity(trade_qty)
            );
            event_log_.push_back(trade);
            
            // Update quantities
            order->remaining_qty = Quantity(order->remaining_qty.get() - trade_qty);
            passive_order->remaining_qty = Quantity(passive_order->remaining_qty.get() - trade_qty);
            ask_level.total_volume = Quantity(ask_level.total_volume.get() - trade_qty);
            
            // Remove filled passive order
            if (passive_order->is_filled()) {
                ask_level.pop();
                order_map_.erase(passive_order->id.get());
            }
            
            // Remove empty level
            if (ask_level.empty()) {
                asks_.erase(asks_.begin());
            }
        }
    }
    
    // Match sell order against bids
    void match_order_sell(std::shared_ptr<Order> order) {
        while (!order->is_filled() && !bids_.empty()) {
            auto& [bid_price, bid_level] = *bids_.begin();
            
            // Check if order crosses
            if (order->price.get() > bid_price) break;
            
            // Match with best bid
            auto passive_order = bid_level.front();
            uint64_t trade_qty = std::min(
                order->remaining_qty.get(), 
                passive_order->remaining_qty.get()
            );
            
            // Generate trade event
            current_time_ = Timestamp(current_time_.get() + 1);
            auto trade = std::make_shared<TradeEvent>(
                current_time_, passive_order->id, order->id,
                Price(bid_price), Quantity(trade_qty)
            );
            event_log_.push_back(trade);
            
            // Update quantities
            order->remaining_qty = Quantity(order->remaining_qty.get() - trade_qty);
            passive_order->remaining_qty = Quantity(passive_order->remaining_qty.get() - trade_qty);
            bid_level.total_volume = Quantity(bid_level.total_volume.get() - trade_qty);
            
            // Remove filled passive order
            if (passive_order->is_filled()) {
                bid_level.pop();
                order_map_.erase(passive_order->id.get());
            }
            
            // Remove empty level
            if (bid_level.empty()) {
                bids_.erase(bids_.begin());
            }
        }
    }
    
    // Add order to book
    void add_to_book(std::shared_ptr<Order> order) {
        if (order->side == Side::BUY) {
            auto it = bids_.find(order->price.get());
            if (it == bids_.end()) {
                bids_.emplace(order->price.get(), LimitLevel(order->price));
                it = bids_.find(order->price.get());
            }
            it->second.add_order(order);
        } else {
            auto it = asks_.find(order->price.get());
            if (it == asks_.end()) {
                asks_.emplace(order->price.get(), LimitLevel(order->price));
                it = asks_.find(order->price.get());
            }
            it->second.add_order(order);
        }
    }
    
    // Remove order from book (simplified - O(N) for clarity)
    void remove_from_book(std::shared_ptr<Order> order) {
        if (order->side == Side::BUY) {
            remove_from_map(bids_, order); // 调用模板函数处理 bids_
        } else {
            remove_from_map(asks_, order); // 调用模板函数处理 asks_
        }
    }
};

#endif // ORDERBOOK_HPP