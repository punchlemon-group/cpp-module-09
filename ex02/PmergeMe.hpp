#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <list>
#include <iterator>
#include <algorithm>

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
        typedef typename Container::iterator Iter;
        typedef typename Container::value_type T;
        Iter first = c.begin();
        Iter last = c.end();
        mergeInsertionSortIter<Iter, T>(first, last);
    }

    template <typename Iter, typename T>
    static void mergeInsertionSortIter(Iter first, Iter last)
    {
        const size_t INSERTION_THRESHOLD = 16;
        typename std::iterator_traits<Iter>::difference_type n = std::distance(first, last);
        if (n <= 1) return;
        if ((size_t)n <= INSERTION_THRESHOLD)
        {
            Iter it = first;
            ++it;
            for (; it != last; ++it)
            {
                Iter it_next = it; ++it_next;
                T val = *it;
                Iter scan = first;
                while (scan != it && !(*scan > val)) ++scan;
                if (scan != it)
                {
                    std::copy_backward(scan, it, it_next);
                    *scan = val;
                }
            }
            return;
        }
        Iter mid = first;
        std::advance(mid, n / 2);
        mergeInsertionSortIter<Iter, T>(first, mid);
        mergeInsertionSortIter<Iter, T>(mid, last);
        std::inplace_merge(first, mid, last);
    }


};

#endif // PMERGEME_HPP
