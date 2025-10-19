#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <list>
#include <iterator>

class PmergeMe
{
public:
    PmergeMe();
    PmergeMe(const PmergeMe &other);
    PmergeMe &operator=(const PmergeMe &other);
    ~PmergeMe();

    template <typename Container>
    void sortContainer(Container &c)
    {
        if (c.size() <= 1) return;
        typedef typename std::iterator_traits<typename Container::iterator>::iterator_category iter_cat;
        sort_impl(c, iter_cat());
    }

private:
    template <typename Container>
    void sort_impl(Container &c, std::random_access_iterator_tag);

    template <typename Container>
    void sort_impl(Container &c, std::bidirectional_iterator_tag);
};

#include "PmergeMe_impl_random_access.hpp"
#include "PmergeMe_impl_bidirectional.hpp"

#endif // PMERGEME_HPP
