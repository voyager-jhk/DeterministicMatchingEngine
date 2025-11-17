#ifndef ORDER_HPP
#define ORDER_HPP

#include "types.hpp"
#include <memory>
#include <queue>

// ============================================================================
// ORDER - Immutable order representation
// ============================================================================

struct Order {
    OrderId id;
    Timestamp timestamp;
    Side side;
    Price price;
    Quantity original_qty;
    Quantity remaining_qty;
    
    Order(OrderId id_, Timestamp ts, Side s, Price p, Quantity q)
        : id(id_), timestamp(ts), side(s), price(p), 
          original_qty(q), remaining_qty(q) {}
    
    bool is_filled() const { 
        return remaining_qty.get() == 0; 
    }
    
    // Invariant: remaining_qty <= original_qty
    bool check_invariants() const {
        return remaining_qty.get() <= original_qty.get();
    }
};

// ============================================================================
// LIMIT LEVEL - Price level with FIFO queue
// ============================================================================

struct LimitLevel {
    Price price;
    std::queue<std::shared_ptr<Order>> orders;
    Quantity total_volume;
    
    explicit LimitLevel(Price p) 
        : price(p), total_volume(Quantity(0)) {}
    
    void add_order(std::shared_ptr<Order> order) {
        orders.push(order);
        total_volume = Quantity(total_volume.get() + order->remaining_qty.get());
    }
    
    std::shared_ptr<Order> front() { 
        return orders.front(); 
    }
    
    void pop() { 
        orders.pop(); 
    }
    
    bool empty() const { 
        return orders.empty(); 
    }
    
    size_t size() const { 
        return orders.size(); 
    }
    
    // Invariant: total_volume equals sum of all order remaining quantities
    bool check_invariants() const {
        uint64_t computed_volume = 0;
        std::queue<std::shared_ptr<Order>> temp = orders;
        
        while (!temp.empty()) {
            auto order = temp.front();
            temp.pop();
            
            if (!order->check_invariants()) {
                return false;
            }
            
            computed_volume += order->remaining_qty.get();
        }
        
        return computed_volume == total_volume.get();
    }
};

#endif // ORDER_HPP