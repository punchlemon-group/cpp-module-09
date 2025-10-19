#include <iostream>
#include <vector>
#include <list>
#include "PmergeMe.hpp"
#include "Utils.hpp"

int exit_error()
{
    std::cerr << "Error" << std::endl;
    return 1;
}

// Compare vector<int> and list<int> for equal length and element-wise equality
bool containers_equal(const std::vector<int> &vec, const std::list<int> &lst)
{
    if (vec.size() != static_cast<size_t>(std::distance(lst.begin(), lst.end()))) return false;
    std::vector<int>::const_iterator vi = vec.begin();
    std::list<int>::const_iterator li = lst.begin();
    for (; vi != vec.end() && li != lst.end(); ++vi, ++li) if (*vi != *li) return false;
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) return exit_error();

    int idx = 1;

    std::vector<int> input;
    if (!parse_input(argc, argv, idx, input)) return exit_error();

    std::vector<int> vec = input;
    std::list<int> lst(input.begin(), input.end());

    PmergeMe vm;
    double tv = 0.0, tl = 0.0;

    std::cout << "Before: ";
    print_container(input);

    if (!measure_sort(vm, vec, tv)) return exit_error();
    if (!measure_sort(vm, lst, tl)) return exit_error();

    if (!containers_equal(vec, lst))
        return exit_error();

    std::cout << "After:  ";
    print_container(vec);

    printResult("std::[vector]", vec, tv);
    printResult("std::[list]  ", lst, tl);

    return 0;
}
