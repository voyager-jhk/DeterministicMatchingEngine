#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <string>

// ============================================================================
// STRONG TYPES - Compile-time Type Safety
// ============================================================================

template<typename T, typename Tag>
struct StrongType {
    T value;
    
    explicit StrongType(T v) : value(v) {}
    
    T get() const { return value; }
    
    bool operator<(const StrongType& other) const { 
        return value < other.value; 
    }
    
    bool operator<=(const StrongType& other) const { 
        return value <= other.value; 
    }
    
    bool operator>(const StrongType& other) const { 
        return value > other.value; 
    }
    
    bool operator>=(const StrongType& other) const { 
        return value >= other.value; 
    }
    
    bool operator==(const StrongType& other) const { 
        return value == other.value; 
    }
    
    bool operator!=(const StrongType& other) const { 
        return value != other.value; 
    }
};

// Type tags for disambiguation
struct OrderIdTag {};
struct PriceTag {};
struct QuantityTag {};
struct TimestampTag {};

// Strong type aliases
using OrderId = StrongType<uint64_t, OrderIdTag>;
using Price = StrongType<double, PriceTag>;
using Quantity = StrongType<uint64_t, QuantityTag>;
using Timestamp = StrongType<uint64_t, TimestampTag>;

// Side enum
enum class Side { 
    BUY, 
    SELL 
};

// Helper function
inline std::string to_string(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

#endif // TYPES_HPP