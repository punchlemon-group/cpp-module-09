#include "RPN.hpp"
#include <sstream>
#include <stdexcept>

RPN::RPN() {}

RPN::RPN(const RPN& other) {
    (void)other;
}

RPN& RPN::operator=(const RPN& other) {
    (void)other;
    return *this;
}

RPN::~RPN() {}

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
            if (token == "+") stk.push(a + b);
            else if (token == "-") stk.push(a - b);
            else if (token == "*") stk.push(a * b);
            else if (token == "/") {
                if (b == 0) throw std::runtime_error("Error");
                stk.push(a / b);
            }
        } else if (token.size() == 1 && isdigit(token[0])) {
            stk.push(token[0] - '0');
        } else {
            throw std::runtime_error("Error");
        }
    }
    if (stk.size() != 1)
        throw std::runtime_error("Error");
    return stk.top();
}
