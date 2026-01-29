#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include "types.hpp"
#include "order.hpp"
#include "events.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>
#include <iostream>
#include <algorithm>
#include <cassert>

// ============================================================================
// ORDER BOOK - HFT Optimized Matching Engine
// ============================================================================

class OrderBook {
private:
    // ------------------------------------------------------------------------
    // Memory Management (Hot Path)
    // ------------------------------------------------------------------------
    ObjectPool<Order> order_pool_;

    // ------------------------------------------------------------------------
    // Data Structures
    // ------------------------------------------------------------------------
    // Bids: highest first (Greater), Asks: lowest first (Less)
    // Key is int64_t (underlying type of Price) to avoid overhead
    std::map<int64_t, LimitLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, LimitLevel, std::less<int64_t>> asks_;

    // Fast O(1) lookup by Order ID
    std::unordered_map<uint64_t, Order*> order_index_;
    
    // Event Log: Stores objects by value (contiguous memory)
    std::vector<Event> event_log_;
    
    Timestamp current_time_;

public:
    // Pre-allocate memory to avoid runtime allocation
    explicit OrderBook(size_t capacity = 1000000) 
        : order_pool_(capacity), current_time_(Timestamp(0)) {
        event_log_.reserve(capacity);
        order_index_.reserve(capacity);
    }

    // ========================================================================
    // PROCESS: NEW ORDER
    // ========================================================================
    void process_new_order(OrderId id, Side side, Price price, Quantity qty) {
        current_time_ = Timestamp(current_time_.get() + 1);
        
        // 1. Log Event (Zero allocation, emplace back)
        event_log_.emplace_back(std::in_place_type<NewOrderEvent>, 
                              current_time_, id, side, price, qty);

        // 2. Allocate Order from Pool (O(1))
        Order* order = order_pool_.allocate();
        if (!order) {
            std::cerr << "CRITICAL: Order Pool Exhausted!\n";
            return;
        }
        // Initialize order in-place
        *order = Order(id, current_time_, side, price, qty);
        
        // 3. Indexing
        order_index_[id.get()] = order;

        // 4. Match logic
        if (side == Side::BUY) {
            match_order_buy(order);
        } else {
            match_order_sell(order);
        }

        // 5. Add remaining to book
        if (!order->is_filled()) {
            add_to_book(order);
        } else {
            order_index_.erase(id.get());
            order_pool_.deallocate(order);
        }
    }

    // ========================================================================
    // PROCESS: CANCEL ORDER (Optimized to O(1))
    // ========================================================================
    void process_cancel(OrderId id) {
        current_time_ = Timestamp(current_time_.get() + 1);
        
        // Log event
        event_log_.emplace_back(std::in_place_type<CancelOrderEvent>, 
                              current_time_, id);

        auto it = order_index_.find(id.get());
        if (it == order_index_.end()) {
            return; // Order not found (already filled or cancelled)
        }

        Order* order = it->second;

        // 1. Remove from LimitLevel (Intrusive Unlink O(1))
        remove_from_level(order);

        // 2. Free memory
        order_index_.erase(it);
        order_pool_.deallocate(order);
    }

    // ========================================================================
    // READ-ONLY ACCESSORS
    // ========================================================================
    std::optional<Price> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return Price(bids_.begin()->first);
    }

    std::optional<Price> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return Price(asks_.begin()->first);
    }

    const std::vector<Event>& get_event_log() const {
        return event_log_;
    }

    // ========================================================================
    // MATCHING LOGIC
    // ========================================================================
private:
    void match_order_buy(Order* aggressive_order) {
        auto it = asks_.begin();
        
        while (it != asks_.end() && !aggressive_order->is_filled()) {
            Price level_price = Price(it->first);
            LimitLevel& level = it->second;

            // Check price crossing
            if (aggressive_order->price.get() < level_price.get()) break;

            // Match against orders in the level
            match_level(aggressive_order, level, level_price);

            // If level empty, remove it
            if (level.empty()) {
                it = asks_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void match_order_sell(Order* aggressive_order) {
        auto it = bids_.begin();
        
        while (it != bids_.end() && !aggressive_order->is_filled()) {
            Price level_price = Price(it->first);
            LimitLevel& level = it->second;

            if (aggressive_order->price.get() > level_price.get()) break;

            match_level(aggressive_order, level, level_price);

            if (level.empty()) {
                it = bids_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void match_level(Order* aggressive, LimitLevel& level, Price match_price) {
        while (!level.empty() && !aggressive->is_filled()) {
            Order* passive = level.front(); // O(1) access

            uint64_t trade_qty = std::min(
                aggressive->remaining_qty.get(),
                passive->remaining_qty.get()
            );

            // 1. Generate Trade Event
            current_time_ = Timestamp(current_time_.get() + 1);
            event_log_.emplace_back(std::in_place_type<TradeEvent>,
                current_time_, passive->id, aggressive->id,
                match_price, Quantity(trade_qty)
            );

            // 2. Update quantities
            aggressive->remaining_qty = Quantity(aggressive->remaining_qty.get() - trade_qty);
            passive->remaining_qty = Quantity(passive->remaining_qty.get() - trade_qty);
            level.total_volume = Quantity(level.total_volume.get() - trade_qty);

            // 3. Handle passive fill
            if (passive->is_filled()) {
                // Remove from book O(1)
                level.pop(); 
                // Remove from index & pool
                order_index_.erase(passive->id.get());
                order_pool_.deallocate(passive);
            }
        }
    }

    // ========================================================================
    // BOOK MANAGEMENT HELPERS
    // ========================================================================
    void add_to_book(Order* order) {
        // Find or create level
        if (order->side == Side::BUY) {
            auto [it, inserted] = bids_.try_emplace(order->price.get(), order->price);
            it->second.add_order(order);
        } else {
            auto [it, inserted] = asks_.try_emplace(order->price.get(), order->price);
            it->second.add_order(order);
        }
    }

    // O(1) removal from doubly-linked list
    void remove_from_level(Order* order) {
        // We need to find the level to update total_volume
        // Since we don't store level* in Order to save memory, we look it up.
        // Optimization: In a real HFT system, Order might store `LimitLevel* parent`
        // at the cost of 8 bytes. Here we look up in map (O(log M)).
        // But the unlinking itself is O(1).
        
        LimitLevel* level = nullptr;
        if (order->side == Side::BUY) {
            auto it = bids_.find(order->price.get());
            if (it != bids_.end()) level = &it->second;
        } else {
            auto it = asks_.find(order->price.get());
            if (it != asks_.end()) level = &it->second;
        }

        if (!level) return; // Should not happen

        // Unlink from list
        if (order->prev) order->prev->next = order->next;
        else level->head = order->next; // Was head

        if (order->next) order->next->prev = order->prev;
        else level->tail = order->prev; // Was tail

        // Update level stats
        level->total_volume = Quantity(level->total_volume.get() - order->remaining_qty.get());
        level->order_count--;

        // Clean up level if empty
        if (level->empty()) {
            if (order->side == Side::BUY) bids_.erase(order->price.get());
            else asks_.erase(order->price.get());
        }
    }
};

#endif