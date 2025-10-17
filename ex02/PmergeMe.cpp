#include "PmergeMe.hpp"
#include <algorithm>

PmergeMe::PmergeMe() {}
PmergeMe::~PmergeMe() {}

// ----- Vector implementation -----
void PmergeMe::sortVector(std::vector<int> &v)
{
    if (v.size() <= 1)
        return;
    mergeInsertSortVector(v, 0, v.size() - 1);
}

void PmergeMe::mergeInsertSortVector(std::vector<int> &v, size_t left, size_t right)
{
    // For small ranges use insertion
    const size_t INSERTION_THRESHOLD = 16;
    if (left >= right)
        return;
    if (right - left + 1 <= INSERTION_THRESHOLD)
    {
        for (size_t i = left + 1; i <= right; ++i)
        {
            int key = v[i];
            // binary insert into [left, i-1]
            size_t l = left, r = i;
            while (l < r)
            {
                size_t m = l + (r - l) / 2;
                if (v[m] <= key)
                    l = m + 1;
                else
                    r = m;
            }
            for (size_t j = i; j > l; --j)
                v[j] = v[j - 1];
            v[l] = key;
        }
        return;
    }

    size_t mid = left + (right - left) / 2;
    mergeInsertSortVector(v, left, mid);
    mergeInsertSortVector(v, mid + 1, right);

    // merge
    std::vector<int> tmp;
    tmp.reserve(right - left + 1);
    size_t i = left, j = mid + 1;
    while (i <= mid && j <= right)
    {
        if (v[i] <= v[j]) tmp.push_back(v[i++]);
        else tmp.push_back(v[j++]);
    }
    while (i <= mid) tmp.push_back(v[i++]);
    while (j <= right) tmp.push_back(v[j++]);
    for (size_t k = 0; k < tmp.size(); ++k)
        v[left + k] = tmp[k];
}

// ----- Deque implementation -----
void PmergeMe::sortDeque(std::deque<int> &d)
{
    if (d.size() <= 1)
        return;
    mergeInsertSortDeque(d, 0, d.size() - 1);
}

void PmergeMe::mergeInsertSortDeque(std::deque<int> &d, int left, int right)
{
    const int INSERTION_THRESHOLD = 16;
    if (left >= right)
        return;
    if (right - left + 1 <= INSERTION_THRESHOLD)
    {
        for (int i = left + 1; i <= right; ++i)
        {
            int key = d[i];
            int l = left, r = i;
            while (l < r)
            {
                int m = l + (r - l) / 2;
                if (d[m] <= key)
                    l = m + 1;
                else
                    r = m;
            }
            for (int j = i; j > l; --j)
                d[j] = d[j - 1];
            d[l] = key;
        }
        return;
    }

    int mid = left + (right - left) / 2;
    mergeInsertSortDeque(d, left, mid);
    mergeInsertSortDeque(d, mid + 1, right);

    // merge
    std::deque<int> tmp;
    tmp.resize(right - left + 1);
    int i = left, j = mid + 1, k = 0;
    while (i <= mid && j <= right)
    {
        if (d[i] <= d[j]) tmp[k++] = d[i++];
        else tmp[k++] = d[j++];
    }
    while (i <= mid) tmp[k++] = d[i++];
    while (j <= right) tmp[k++] = d[j++];
    for (int p = 0; p < (int)tmp.size(); ++p)
        d[left + p] = tmp[p];
}
