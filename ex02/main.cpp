#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "PmergeMe.hpp"

static bool is_number(const std::string &s)
{
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (!std::isdigit(s[i]))
            return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Error" << std::endl;
        return 1;
    }

    std::vector<int> input;
    for (int i = 1; i < argc; ++i)
    {
        std::string s(argv[i]);
        if (!is_number(s))
        {
            std::cerr << "Error" << std::endl;
            return 1;
        }
        long val;
        std::istringstream iss(s);
        iss >> val;
        if (val > 2147483647 || val <= 0)
        {
            std::cerr << "Error" << std::endl;
            return 1;
        }
        input.push_back(static_cast<int>(val));
    }

    std::cout << "Before: ";
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (i) std::cout << " ";
        std::cout << input[i];
    }
    std::cout << std::endl;

    // prepare containers
    std::vector<int> vec = input;
    std::deque<int> deq(input.begin(), input.end());

    PmergeMe vm;

    // time vector sort using gettimeofday (microseconds)
    struct timeval t1, t2, t3, t4;
    if (gettimeofday(&t1, NULL) != 0) { std::cerr << "Error" << std::endl; return 1; }
    try { vm.sortVector(vec); } catch(...) { std::cerr << "Error" << std::endl; return 1; }
    if (gettimeofday(&t2, NULL) != 0) { std::cerr << "Error" << std::endl; return 1; }
    double time_vec = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_usec - t1.tv_usec);

    // time deque sort
    if (gettimeofday(&t3, NULL) != 0) { std::cerr << "Error" << std::endl; return 1; }
    try { vm.sortDeque(deq); } catch(...) { std::cerr << "Error" << std::endl; return 1; }
    if (gettimeofday(&t4, NULL) != 0) { std::cerr << "Error" << std::endl; return 1; }
    double time_deq = (t4.tv_sec - t3.tv_sec) * 1e6 + (t4.tv_usec - t3.tv_usec);

    std::cout << "After: ";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i) std::cout << " ";
        std::cout << vec[i];
    }
    std::cout << std::endl;

    std::cout << "Time to process a range of " << vec.size()
              << " elements with std::vector : " << time_vec << " us" << std::endl;
    std::cout << "Time to process a range of " << deq.size()
              << " elements with std::deque  : " << time_deq << " us" << std::endl;

    return 0;
}
