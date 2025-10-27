#pragma once

# include <iostream>

class CounterUint {
public:
    CounterUint();
    CounterUint(unsigned int value);
    CounterUint(const CounterUint& src);
    CounterUint& operator=(const CounterUint& src);
    ~CounterUint();
    bool operator<(const CounterUint& other) const;
    bool operator>(const CounterUint& other) const;
    bool operator<=(const CounterUint& other) const;
    bool operator>=(const CounterUint& other) const;
    bool operator==(const CounterUint& other) const;
    bool operator!=(const CounterUint& other) const;
    unsigned int getValue() const;
    static unsigned int getCompareCount();
    static void resetCompareCount();
private:
    unsigned int value_;
    static unsigned int compareCount;
};

bool operator<(unsigned int lhs, const CounterUint& rhs);
bool operator>(unsigned int lhs, const CounterUint& rhs);
bool operator<=(unsigned int lhs, const CounterUint& rhs);
bool operator>=(unsigned int lhs, const CounterUint& rhs);
bool operator==(unsigned int lhs, const CounterUint& rhs);
bool operator!=(unsigned int lhs, const CounterUint& rhs);
std::ostream& operator<<(std::ostream& lhs, const CounterUint& rhs);
