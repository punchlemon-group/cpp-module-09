#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <list>

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
        typedef typename Container::value_type value_type;
        std::vector<value_type> tmp(c.begin(), c.end());
        mergeInsertionSortInPlace<value_type>(tmp, 0, tmp.size() - 1);
        typename Container::iterator it = c.begin();
        for (size_t i = 0; i < tmp.size(); ++i, ++it) *it = tmp[i];
    }

private:
    template <typename T>
    static void mergeInsertionSortInPlace(std::vector<T> &v, size_t left, size_t right)
    {
        const size_t INSERTION_THRESHOLD = 16;
        if (left >= right) return;
        if (right - left + 1 <= INSERTION_THRESHOLD)
        {
            // Binary insertion sort on small ranges
            for (size_t i = left + 1; i <= right; ++i)
            {
                T key = v[i];
                size_t l = left, r = i;
                while (l < r)
                {
                    size_t m = l + (r - l) / 2;
                    if (v[m] <= key) l = m + 1; else r = m;
                }
                for (size_t j = i; j > l; --j) v[j] = v[j - 1];
                v[l] = key;
            }
            return;
        }
        size_t mid = left + (right - left) / 2;
        mergeInsertionSortInPlace(v, left, mid);
        mergeInsertionSortInPlace(v, mid + 1, right);
        std::vector<T> tmp;
        tmp.reserve(right - left + 1);
        size_t i = left, j = mid + 1;
        while (i <= mid && j <= right) { if (v[i] <= v[j]) tmp.push_back(v[i++]); else tmp.push_back(v[j++]); }
        while (i <= mid) tmp.push_back(v[i++]);
        while (j <= right) tmp.push_back(v[j++]);
        for (size_t k = 0; k < tmp.size(); ++k) v[left + k] = tmp[k];
    }
};

#endif // PMERGEME_HPP
