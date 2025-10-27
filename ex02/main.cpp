#include <iostream>
#include "PmergeMe.hpp"
#include "Utils.hpp"
#include "CounterUint.hpp"


#ifdef DEFINE_TEST
const bool test_mode = true;
#else
const bool test_mode = false;
#endif

int exit_error()
{
    std::cerr << "Error" << std::endl;
    return 1;
}

bool containers_equal(const std::vector<CounterUint> &vec, const std::deque<CounterUint> &deq)
{
    if (vec.size() != deq.size()) return false;
    return std::equal(vec.begin(), vec.end(), deq.begin());
}

int main(int argc, char **argv)
{
    if (argc < 2) return exit_error();

    int idx = 1;

    std::vector<int> input;
    if (!parse_input(argc, argv, idx, input)) return exit_error();

    std::vector<CounterUint> vec;
    std::deque<CounterUint> deq;
    vec.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        CounterUint temp(static_cast<unsigned int>(input[i]));
        vec.push_back(temp);
        deq.push_back(temp);
    }

    PmergeMe vm;
    double tv = 0.0, tl = 0.0;

    std::cout << "Before: ";
    print_container(input);

    CounterUint::resetCompareCount();
    if (!measure_sort(vm, vec, tv)) return exit_error();
    unsigned int vecComps = CounterUint::getCompareCount();

    CounterUint::resetCompareCount();
    if (!measure_sort(vm, deq, tl)) return exit_error();
    unsigned int deqComps = CounterUint::getCompareCount();

    if (!containers_equal(vec, deq))
    {
        std::cout << "vec:    "; print_container(vec);
        std::cout << "deq:    "; print_container(deq);
        return exit_error();
    }

    std::cout << "After:  ";
    print_container(vec);

    printResult("std::[vector]", vec, tv);
    printResult("std::[deque] ", deq, tl);
    if (test_mode) {
        std::cout << "Number of comparisons: " << vecComps << std::endl;
        std::cout << "Number of comparisons: " << deqComps << std::endl;
    }

    return 0;
}
