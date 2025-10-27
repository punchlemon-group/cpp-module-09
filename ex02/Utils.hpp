#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "PmergeMe.hpp"
#include <cmath>

double get_time_us();
bool parse_input(int argc, char **argv, int start, std::vector<int> &out);

template <typename Container>
void print_container(const Container &c)
{
    bool first = true;
    for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it)
    {
        if (!first) std::cout << " ";
        std::cout << *it;
        first = false;
    }
    std::cout << std::endl;
}

template <typename Container>
bool measure_sort(PmergeMe &pm, Container &c, double &out_time_us)
{
    double t1 = get_time_us();
    try { pm.sortContainer(c); }
    catch (const std::exception &e) { std::cerr << "Exception during sort: " << e.what() << std::endl; return false; }
    catch (...) { std::cerr << "Unknown exception during sort" << std::endl; return false; }
    double t2 = get_time_us();
    out_time_us = t2 - t1;
    return true;
}

template <typename Container>
void printResult(const char *label, const Container &c, double time_us)
{
    // Ensure consistent floating formatting for times
    std::cout << std::fixed << std::setprecision(5);

    size_t n = c.size();
    int eff_width = std::max(3, (n == 0) ? 1 : (int)std::floor(std::log10(static_cast<double>(n))) + 1);

    std::cout << "Time to process a range of ";
    std::cout << std::setw(eff_width) << std::right << c.size();
    std::cout << " elements with " << label << " : " << time_us << " us" << std::endl;
}
