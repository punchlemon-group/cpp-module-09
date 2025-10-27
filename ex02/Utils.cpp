#include "Utils.hpp"

#include <sstream>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <ctype.h>
#if defined(__APPLE__)
# include <mach/mach_time.h>
#endif

double get_time_us()
{
#if defined(__APPLE__)
    static mach_timebase_info_data_t tb = {0,0};
    if (tb.denom == 0) mach_timebase_info(&tb);
    uint64_t t = mach_absolute_time();
    double ns = (double)t * (double)tb.numer / (double)tb.denom;
    return ns / 1000.0; // microseconds
#else
    struct timespec ts;
# if defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
    }
# endif
    return 0.0;
#endif
}

bool parse_input(int argc, char **argv, int start, std::vector<int> &out)
{
    out.clear();
    for (int i = start; i < argc; ++i)
    {
        const char *raw = argv[i];
        if (!raw) return false;
        std::string s(raw);
        if (s.empty() || s.size() > 10) return false;
        for (std::string::size_type j = 0; j < s.size(); ++j)
            if (!std::isdigit(static_cast<unsigned char>(s[j]))) return false;
        errno = 0;
        char *endptr = NULL;
        long val = strtol(s.c_str(), &endptr, 10);
        if (errno == ERANGE || endptr == s || val > 2147483647L || val <= 0) return false;
        out.push_back(static_cast<int>(val));
    }
    return true;
}
