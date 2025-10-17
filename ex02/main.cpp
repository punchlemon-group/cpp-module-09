#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <iomanip>
#if defined(__APPLE__)
# include <mach/mach_time.h>
#endif
#include "PmergeMe.hpp"

#ifndef SHOW_RAW_TICKS
#define SHOW_RAW_TICKS 1
#endif

// high-resolution timer (returns microseconds). May also return raw ticks/resolution.
static double get_time_us(uint64_t *out_raw = NULL, double *out_res_ns = NULL, int *out_raw_is_ticks = NULL)
{
#if defined(__APPLE__)
    static mach_timebase_info_data_t tb = {0,0};
    if (tb.denom == 0) mach_timebase_info(&tb);
    uint64_t t = mach_absolute_time();
    double ns = (double)t * (double)tb.numer / (double)tb.denom;
    if (out_raw) *out_raw = t;
    if (out_res_ns) *out_res_ns = (double)tb.numer / (double)tb.denom; // ns per tick
    if (out_raw_is_ticks) *out_raw_is_ticks = 1;
    return ns / 1000.0; // microseconds
#else
    struct timespec ts;
# if defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        if (out_raw)
            *out_raw = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        if (out_res_ns)
        {
            struct timespec res;
            if (clock_getres(CLOCK_MONOTONIC, &res) == 0)
                *out_res_ns = (double)res.tv_sec * 1e9 + (double)res.tv_nsec;
            else
                *out_res_ns = 0.0;
        }
        if (out_raw_is_ticks) *out_raw_is_ticks = 0;
        return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
    }
# endif
    // No fallback available; return 0.0 to indicate failure
    return 0.0;
#endif
}

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

template <typename Container>
void printResult(const char *label, const Container &c, double time_us,
                 bool show_raw, int is_ticks, uint64_t raw1, uint64_t raw2, double res_ns)
{
    std::cout << "Time to process a range of " << c.size()
              << " elements with " << label << " : " << time_us;
    if (show_raw)
    {
        if (is_ticks)
        {
            uint64_t ticks = raw2 - raw1;
            std::cout << " | ticks=" << ticks << " | ns/tick=" << res_ns;
        }
        else
        {
            uint64_t diff_ns = raw2 - raw1;
            std::cout << " | diff=" << diff_ns << " ns | res=" << res_ns << " ns";
        }
    }
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Error" << std::endl;
        return 1;
    }


    const bool show_raw = SHOW_RAW_TICKS;
    int idx = 1;

    std::vector<int> input;
    for (int i = idx; i < argc; ++i)
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

    std::vector<int> vec = input;
    std::list<int> lst(input.begin(), input.end());

    PmergeMe vm;
    uint64_t raw_v1 = 0, raw_v2 = 0, raw_d1 = 0, raw_d2 = 0;
    double res_v = 0.0, res_d = 0.0;
    int is_ticks_v = 0, is_ticks_d = 0;
    double t1 = get_time_us(&raw_v1, &res_v, &is_ticks_v);
    try { vm.sortContainer(vec); } catch(...) { std::cerr << "Error" << std::endl; return 1; }
    double t2 = get_time_us(&raw_v2, NULL, NULL);
    double time_vec = t2 - t1;

    double t3 = get_time_us(&raw_d1, &res_d, &is_ticks_d);
    try { vm.sortContainer(lst); } catch(...) { std::cerr << "Error" << std::endl; return 1; }
    double t4 = get_time_us(&raw_d2, NULL, NULL);
    double time_list = t4 - t3;

    std::cout << "After: ";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i) std::cout << " ";
        std::cout << vec[i];
    }
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(5);
    printResult("std::[vector]", vec, time_vec, show_raw, is_ticks_v, raw_v1, raw_v2, res_v);
    printResult("std::[list]", lst, time_list, show_raw, is_ticks_d, raw_d1, raw_d2, res_d);

    return 0;
}
