#include "PmergeMe.hpp"
#include <algorithm>

PmergeMe::PmergeMe() {}

PmergeMe::PmergeMe(const PmergeMe &other)
{
    // No owned resources to copy; default behavior is sufficient.
    (void)other;
}

PmergeMe &PmergeMe::operator=(const PmergeMe &other)
{
    if (this == &other) return *this;
    // No owned resources; nothing to assign.
    return *this;
}

PmergeMe::~PmergeMe() {}
