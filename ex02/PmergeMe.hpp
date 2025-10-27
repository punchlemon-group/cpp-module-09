#pragma once

#include <iterator>
#include "ElementSequence.hpp"

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
    void sort_impl(Container &c, std::random_access_iterator_tag) {
        ElementSequence<Container> seq(c);
        seq.sort();
        c = seq.getResult();
    }

    // template <typename Container>
    // void sort_impl(Container &c, std::bidirectional_iterator_tag);
};
