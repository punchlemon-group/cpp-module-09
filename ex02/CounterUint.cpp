#include "CounterUint.hpp"

unsigned int CounterUint::compareCount = 0;

CounterUint::CounterUint() : value_(0) {
}

CounterUint::CounterUint(unsigned int value) : value_(value) {
}

CounterUint::CounterUint(const CounterUint& src) : value_(src.value_) {
}

CounterUint& CounterUint::operator=(const CounterUint& src) {
    if (this != &src) {
        value_ = src.value_;
    }
    return *this;
}

CounterUint::~CounterUint() {
}

bool CounterUint::operator<(const CounterUint& other) const {
    ++compareCount;
    return value_ < other.value_;
}

bool CounterUint::operator>(const CounterUint& other) const {
    ++compareCount;
    return value_ > other.value_;
}

bool CounterUint::operator<=(const CounterUint& other) const {
    ++compareCount;
    return value_ <= other.value_;
}

bool CounterUint::operator>=(const CounterUint& other) const {
    ++compareCount;
    return value_ >= other.value_;
}

bool CounterUint::operator==(const CounterUint& other) const {
    ++compareCount;
    return value_ == other.value_;
}

bool CounterUint::operator!=(const CounterUint& other) const {
    ++compareCount;
    return value_ != other.value_;
}

unsigned int CounterUint::getValue() const {
    return value_;
}

unsigned int CounterUint::getCompareCount() {
    return compareCount;
}

void CounterUint::resetCompareCount() {
    compareCount = 0;
}

// When the left operand is an unsigned int,
//  create a temporary CounterUint and delegate to the member operator
// This ensures it is counted exactly once for each comparison
bool operator<(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) < rhs;
}

bool operator>(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) > rhs;
}

bool operator<=(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) <= rhs;
}

bool operator>=(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) >= rhs;
}

bool operator==(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) == rhs;
}

bool operator!=(unsigned int lhs, const CounterUint& rhs) {
    return CounterUint(lhs) != rhs;
}

std::ostream& operator<<(std::ostream& lhs, const CounterUint& rhs) {
    lhs << rhs.getValue();
    return lhs;
}
