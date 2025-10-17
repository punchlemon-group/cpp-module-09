#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <deque>

class PmergeMe
{
public:
    PmergeMe();
    ~PmergeMe();

    void sortVector(std::vector<int> &v);
    void sortDeque(std::deque<int> &d);

private:
    // internal implementations
    void mergeInsertSortVector(std::vector<int> &v, size_t left, size_t right);
    void binaryInsertVector(std::vector<int> &v, int value, size_t left, size_t right);

    void mergeInsertSortDeque(std::deque<int> &d, int left, int right);
    void binaryInsertDeque(std::deque<int> &d, int value, int left, int right);
};

#endif
