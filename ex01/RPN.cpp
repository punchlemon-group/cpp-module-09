#include "RPN.hpp"
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cctype>
#include <climits>

RPN::RPN() {}

RPN::RPN(const RPN& other) {
    (void)other;
}

RPN& RPN::operator=(const RPN& other) {
    (void)other;
    return *this;
}

RPN::~RPN() {}

static int safe_add(int a, int b);
static int safe_sub(int a, int b);
static int safe_mul(int a, int b);
static int safe_div(int a, int b);

int RPN::evaluate(const std::string& expr) {
    std::istringstream iss(expr);
    std::stack<int> stk;
    std::string token;

    while (iss >> token) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stk.size() < 2)
                throw std::runtime_error("Error");
            int b = stk.top(); stk.pop();
            int a = stk.top(); stk.pop();

            if (token == "+") stk.push(safe_add(a, b));
            else if (token == "-") stk.push(safe_sub(a, b));
            else if (token == "*") stk.push(safe_mul(a, b));
            else if (token == "/") stk.push(safe_div(a, b));

        } else if (token.size() == 1 && std::isdigit(static_cast<unsigned char>(token[0]))) {
            stk.push(token[0] - '0');
        } else {
            throw std::runtime_error("Error");
        }
    }

    if (stk.size() != 1)
        throw std::runtime_error("Error");
    return stk.top();
}

static int safe_add(int a, int b) {
    const int INT_MAX_V = std::numeric_limits<int>::max();
    const int INT_MIN_V = std::numeric_limits<int>::min();
    if (b > 0 && a > INT_MAX_V - b) throw std::runtime_error("Error");
    if (b < 0 && a < INT_MIN_V - b) throw std::runtime_error("Error");
    return a + b;
}

static int safe_sub(int a, int b) {
    const int INT_MAX_V = std::numeric_limits<int>::max();
    const int INT_MIN_V = std::numeric_limits<int>::min();
    if (b > 0 && a < INT_MIN_V + b) throw std::runtime_error("Error");
    if (b < 0 && a > INT_MAX_V + b) throw std::runtime_error("Error");
    return a - b;
}

static int safe_mul(int a, int b) {
    const int INT_MAX_V = std::numeric_limits<int>::max();
    const int INT_MIN_V = std::numeric_limits<int>::min();
    if (a == 0 || b == 0) return 0;
    if (a == INT_MIN_V) {
        if (b == 1) return INT_MIN_V;
        throw std::runtime_error("Error");
    }
    if (b == INT_MIN_V) {
        if (a == 1) return INT_MIN_V;
        throw std::runtime_error("Error");
    }

    int abs_a = a < 0 ? -a : a;
    int abs_b = b < 0 ? -b : b;
    if (abs_a > INT_MAX_V / abs_b) throw std::runtime_error("Error");
    return a * b;
}

static int safe_div(int a, int b) {
    const int INT_MIN_V = std::numeric_limits<int>::min();
    if (b == 0) throw std::runtime_error("Error");
    if (a == INT_MIN_V && b == -1) throw std::runtime_error("Error");
    return a / b;
}
