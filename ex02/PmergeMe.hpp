
#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <cstddef>

// Minimal, single-definition PmergeMe header.
// OCF special-members are declared here and defined in PmergeMe.cpp.
class PmergeMe
{
public:
    PmergeMe();
    PmergeMe(const PmergeMe &other);
    PmergeMe &operator=(const PmergeMe &other);
    ~PmergeMe();

    // Generic container sort entry point (container-agnostic).
    template <typename Container>
    void sortContainer(Container &c)
    {
        if (c.size() <= 1) return;
        // Tag dispatch on iterator category to pick the right implementation
        typedef typename std::iterator_traits<typename Container::iterator>::iterator_category iter_cat;
        sort_impl(c, iter_cat());
    }

    // Random-access iterator implementation (e.g. std::vector)
    template <typename Container>
    void sort_impl(Container &c, std::random_access_iterator_tag)
    {
        typedef typename Container::value_type Elem;
        size_t n = c.size();
        if (n <= 1) return;

        // Work with indices only (no copying container elements into a vector)
        std::vector<size_t> pairs_idx_small; // pend indices (smaller in each pair)
        std::vector< std::pair<size_t, size_t> > pairs; // (small_idx, large_idx)
        size_t i = 0;
        for (; i + 1 < n; i += 2)
        {
            size_t a = i;
            size_t b = i + 1;
            if (c[a] <= c[b])
            {
                pairs.push_back(std::make_pair(a, b));
                pairs_idx_small.push_back(a);
            }
            else
            {
                pairs.push_back(std::make_pair(b, a));
                pairs_idx_small.push_back(b);
            }
        }
        bool has_straggler = false;
        size_t straggler_index = 0;
        if (i < n) { has_straggler = true; straggler_index = i; }

        // Build main_chain indices (larger elements of pairs)
        std::vector<size_t> main_chain_indices;
        for (size_t j = 0; j < pairs.size(); ++j) main_chain_indices.push_back(pairs[j].second);

        // Recursively sort the mains by constructing a temporary container of their values.
        // To avoid converting arbitrary container to std::vector, we only create a temporary
        // std::vector of Elem for the mains (allowed here because this is internal to random access case)
        std::vector<Elem> main_values;
        for (size_t j = 0; j < main_chain_indices.size(); ++j) main_values.push_back(c[ main_chain_indices[j] ]);

        // Use the existing fordJohnsonOrder on main_values (it returns indices into main_values)
        std::vector<size_t> mains_order = fordJohnsonOrder<Elem>(main_values);

        // Reorder main_chain_indices and pend indices according to mains_order
        std::vector<size_t> sorted_main_chain;
        std::vector<size_t> pend_in_order;
        for (size_t k = 0; k < mains_order.size(); ++k)
        {
            size_t idx_in_pairs = mains_order[k];
            sorted_main_chain.push_back(main_chain_indices[idx_in_pairs]);
            pend_in_order.push_back(pairs_idx_small[idx_in_pairs]);
        }

        // If no pendings, just reorder according to sorted_main_chain
        if (pend_in_order.empty())
        {
            Container tmp;
            for (size_t r = 0; r < sorted_main_chain.size(); ++r) tmp.push_back(c[ sorted_main_chain[r] ]);
            if (has_straggler) tmp.push_back(c[straggler_index]);
            c.swap(tmp);
            return;
        }

        // Build result indices (original indices into c)
        std::vector<size_t> result_idx = sorted_main_chain;

        // Insert first pend at front
        result_idx.insert(result_idx.begin(), pend_in_order[0]);

        // Jacobsthal sequence and insertion order
        size_t m = pend_in_order.size();
        std::vector<size_t> t_vals;
        for (size_t k = 1; ; ++k)
        {
            unsigned long pow2 = 1UL << (k + 1);
            int sign = (k % 2 == 0) ? 1 : -1;
            unsigned long t = (pow2 + (unsigned long)sign) / 3UL;
            t_vals.push_back((size_t)t);
            if (t >= m) break;
        }
        std::vector<size_t> insertion_order;
        for (size_t g = 1; g < t_vals.size(); ++g)
        {
            size_t start = t_vals[g - 1] + 1;
            size_t end = t_vals[g] < m ? t_vals[g] : m;
            if (start > end) continue;
            for (size_t x = end; ; --x)
            {
                insertion_order.push_back(x);
                if (x == start) break;
            }
        }

        // Insert remaining pendings following insertion_order
        for (size_t ii = 0; ii < insertion_order.size(); ++ii)
        {
            size_t pid = insertion_order[ii] - 1; // 0-based
            size_t pend_idx = pend_in_order[pid];
            Elem pend_val = c[pend_idx];
            size_t assoc_main_orig = sorted_main_chain[pid];

            // find assoc_main_orig position in result_idx
            size_t pos = 0;
            for (; pos < result_idx.size(); ++pos) if (result_idx[pos] == assoc_main_orig) break;

            if (pos == 0)
            {
                result_idx.insert(result_idx.begin(), pend_idx);
            }
            else
            {
                // binary search in [0, pos) comparing c[result_idx[mid]] < pend_val
                size_t lo = 0, hi = pos;
                while (lo < hi)
                {
                    size_t mid = (lo + hi) / 2;
                    if (c[ result_idx[mid] ] < pend_val) lo = mid + 1; else hi = mid;
                }
                result_idx.insert(result_idx.begin() + lo, pend_idx);
            }
        }

        // Insert straggler if present
        if (has_straggler)
        {
            Elem sval = c[straggler_index];
            size_t lo = 0, hi = result_idx.size();
            while (lo < hi)
            {
                size_t mid = (lo + hi) / 2;
                if (c[ result_idx[mid] ] < sval) lo = mid + 1; else hi = mid;
            }
            result_idx.insert(result_idx.begin() + lo, straggler_index);
        }

        // Build tmp container in sorted order and swap
        Container tmp;
        for (size_t r = 0; r < result_idx.size(); ++r) tmp.push_back(c[ result_idx[r] ]);
        c.swap(tmp);
    }

    // Bidirectional iterator implementation (e.g. std::list)
    template <typename Container>
    void sort_impl(Container &c, std::bidirectional_iterator_tag)
    {
        typedef typename Container::value_type Elem;
        // We'll operate on lists of values; this overload assumes Container is a list-like container.
        // If container is too small, nothing to do
        if (c.size() <= 1) return;

        // Handle straggler (odd size) by removing last element up-front and saving it
        bool had_straggler = false;
        Elem straggler_val = Elem();
        typename Container::size_type orig_size = c.size();
        if ((orig_size % 2) == 1)
        {
            typename Container::iterator last = c.end(); --last;
            straggler_val = *last;
            c.erase(last);
            had_straggler = true;
        }

        // Build pairs by traversing iterators two at a time and copying values into a std::list.
        std::list< std::pair<Elem, Elem> > pairs_list; // (main_value, pend_value)
        typename Container::iterator it = c.begin();
        while (it != c.end())
        {
            typename Container::iterator it1 = it++; // first
            typename Container::iterator it2 = it++; // second (safe because size is even)
            Elem a = *it1; Elem b = *it2;
            if (a <= b) pairs_list.push_back(std::make_pair(b, a)); else pairs_list.push_back(std::make_pair(a, b));
        }

        // Build main_chain list (larger values)
        std::list<Elem> main_chain;
        for (typename std::list< std::pair<Elem,Elem> >::iterator pit = pairs_list.begin(); pit != pairs_list.end(); ++pit)
        {
            main_chain.push_back(pit->first);
        }

        // Recursively sort the main_chain using the same tag dispatch
        sort_impl(main_chain, std::bidirectional_iterator_tag());

        // Map pends to mains according to sorted mains: for each value in main_chain, find and extract matching pair from pairs_list
        std::list<Elem> pend_in_order;
        for (typename std::list<Elem>::iterator mit = main_chain.begin(); mit != main_chain.end(); ++mit)
        {
            // find a pair in pairs_list with first == *mit
            for (typename std::list< std::pair<Elem,Elem> >::iterator pit = pairs_list.begin(); pit != pairs_list.end(); ++pit)
            {
                if (pit->first == *mit)
                {
                    pend_in_order.push_back(pit->second);
                    pairs_list.erase(pit);
                    break;
                }
            }
        }

        // Construct result list starting with sorted mains
        std::list<Elem> result;
        for (typename std::list<Elem>::iterator mit = main_chain.begin(); mit != main_chain.end(); ++mit) result.push_back(*mit);

        if (pend_in_order.empty())
        {
            // No pends: replace container with result
            c.clear();
            for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit);
            return;
        }

        // Insert first pend at front
        result.push_front(pend_in_order.front());

        // Prepare insertion order using Jacobsthal sequence (pend_in_order size = m)
        size_t m = 0;
        for (typename std::list<Elem>::iterator itx = pend_in_order.begin(); itx != pend_in_order.end(); ++itx) ++m;
        std::vector<size_t> t_vals;
        for (size_t k = 1; ; ++k)
        {
            unsigned long pow2 = 1UL << (k + 1);
            int sign = (k % 2 == 0) ? 1 : -1;
            unsigned long t = (pow2 + (unsigned long)sign) / 3UL;
            t_vals.push_back((size_t)t);
            if (t >= m) break;
        }
        std::vector<size_t> insertion_order;
        for (size_t g = 1; g < t_vals.size(); ++g)
        {
            size_t start = t_vals[g - 1] + 1;
            size_t end = t_vals[g] < m ? t_vals[g] : m;
            if (start > end) continue;
            for (size_t x = end; ; --x)
            {
                insertion_order.push_back(x);
                if (x == start) break;
            }
        }

        // Convert pend_in_order to vector for indexed access (allowed local copy of pend values)
        std::vector<Elem> pend_vec;
        for (typename std::list<Elem>::iterator pit = pend_in_order.begin(); pit != pend_in_order.end(); ++pit) pend_vec.push_back(*pit);

        // Also keep a vector of mains in the original sorted order for association
        std::vector<Elem> mains_vec;
        for (typename std::list<Elem>::iterator mit = main_chain.begin(); mit != main_chain.end(); ++mit) mains_vec.push_back(*mit);

        // Process insertion_order: for each pend indexed by pid (1-based), find assoc main (mains_vec[pid-1]) and insert pend into result before or in proper spot via lower_bound in [begin, assoc_it)
        for (size_t ii = 0; ii < insertion_order.size(); ++ii)
        {
            size_t pid = insertion_order[ii] - 1; // 0-based
            Elem pend_val = pend_vec[pid];
            Elem assoc_main = mains_vec[pid];

            // find assoc_main in result (first occurrence)
            typename std::list<Elem>::iterator assoc_it = result.begin();
            for (; assoc_it != result.end(); ++assoc_it) if (*assoc_it == assoc_main) break;

            if (assoc_it == result.begin())
            {
                result.push_front(pend_val);
                continue;
            }

            // Compute distance from begin to assoc_it
            size_t len = 0; typename std::list<Elem>::iterator tmpit = result.begin();
            for (; tmpit != assoc_it; ++tmpit) ++len;

            // Binary search by index using std::advance to get mid iterator each time (inefficient for list but required)
            size_t lo = 0, hi = len;
            while (lo < hi)
            {
                size_t mid = (lo + hi) / 2;
                typename std::list<Elem>::iterator midit = result.begin();
                std::advance(midit, mid);
                if (*midit < pend_val) lo = mid + 1; else hi = mid;
            }
            typename std::list<Elem>::iterator insert_it = result.begin();
            std::advance(insert_it, lo);
            result.insert(insert_it, pend_val);
        }

        // If we removed a straggler earlier, insert it now across the whole result
        if (had_straggler)
        {
            Elem sval = straggler_val;
            // compute length
            size_t len = 0; for (typename std::list<Elem>::iterator itx = result.begin(); itx != result.end(); ++itx) ++len;
            size_t lo = 0, hi = len;
            while (lo < hi)
            {
                size_t mid = (lo + hi) / 2;
                typename std::list<Elem>::iterator midit = result.begin(); std::advance(midit, mid);
                if (*midit < sval) lo = mid + 1; else hi = mid;
            }
            typename std::list<Elem>::iterator insert_it = result.begin(); std::advance(insert_it, lo);
            result.insert(insert_it, sval);
        }

        // Replace original container contents with result
        c.clear();
        for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit);
    }

    // Expose a debug entry to call the internal order generator from tests.
    template <typename T>
    static std::vector<size_t> debugFordJohnsonOrder(const std::vector<T> &values)
    {
        return fordJohnsonOrder<T>(values);
    }

private:
    // Comparator that compares indices by looking up values (stable_sort-compatible)
    template <typename T>
    struct LessByValue
    {
        const std::vector<T> *vals;
        LessByValue(const std::vector<T> *v): vals(v) {}
        bool operator()(size_t a, size_t b) const { return (*vals)[a] < (*vals)[b]; }
    };

    // Baseline order: stable sort indices by value. Replace this function later
    // with a tested Ford-Johnson implementation in small steps.
    template <typename T>
    static std::vector<size_t> fordJohnsonOrder(const std::vector<T> &values)
    {
        // Ford-Johnson (merge-insertion) implementation (C++98)
        size_t n = values.size();
        if (n == 0) return std::vector<size_t>();
        if (n == 1) { std::vector<size_t> r; r.push_back(0); return r; }

        // Step 1: Pair formation
        // pairs: for each pair store (smaller_index, larger_index)
        std::vector< std::pair<size_t, size_t> > pairs;
        std::vector<size_t> pend_indices; // smaller indices (pendings), in pair order
        size_t i = 0;
        for (; i + 1 < n; i += 2)
        {
            size_t a = i;
            size_t b = i + 1;
            if (values[a] <= values[b])
            {
                pairs.push_back(std::make_pair(a, b));
                pend_indices.push_back(a);
            }
            else
            {
                pairs.push_back(std::make_pair(b, a));
                pend_indices.push_back(b);
            }
        }
        bool has_straggler = false;
        size_t straggler_index = 0;
        if (i < n) { has_straggler = true; straggler_index = i; }

        // Step 2: Build main_chain indices (the larger elements of each pair)
        std::vector<size_t> main_chain_indices;
        for (size_t j = 0; j < pairs.size(); ++j)
            main_chain_indices.push_back(pairs[j].second);

        // Recurse on the main chain values
        std::vector<T> main_values;
        for (size_t j = 0; j < main_chain_indices.size(); ++j)
            main_values.push_back(values[ main_chain_indices[j] ]);

        // Recursive call: returns an order of indices into main_values
        std::vector<size_t> mains_order = fordJohnsonOrder<T>(main_values);

        // Reorder main_chain_indices and pend_indices according to mains_order
        std::vector<size_t> sorted_main_chain;
        std::vector<size_t> pend_in_order;
        for (size_t k = 0; k < mains_order.size(); ++k)
        {
            size_t idx_in_pairs = mains_order[k];
            sorted_main_chain.push_back(main_chain_indices[idx_in_pairs]);
            pend_in_order.push_back(pend_indices[idx_in_pairs]);
        }

        // Step 3: Insert pendings into the sorted main chain
        // If there are no pendings, return the mains (we will insert straggler later)
        if (pend_in_order.empty())
        {
            // result currently is sorted_main_chain
            return sorted_main_chain;
        }

        // result will hold indices into original values
        std::vector<size_t> result = sorted_main_chain;

        // insert first pend at front
        result.insert(result.begin(), pend_in_order[0]);

        // Build Jacobsthal-related t_k values and insertion_order (1-based pend indices)
        size_t m = pend_in_order.size();
        std::vector<size_t> t_vals;
        for (size_t k = 1; ; ++k)
        {
            unsigned long pow2 = 1UL << (k + 1); // 2^(k+1)
            int sign = (k % 2 == 0) ? 1 : -1;
            unsigned long t = (pow2 + (unsigned long)sign) / 3UL;
            t_vals.push_back((size_t)t);
            if (t >= m) break;
        }

        std::vector<size_t> insertion_order; // will contain 1-based pend indices (excluding the first)
        for (size_t g = 1; g < t_vals.size(); ++g)
        {
            size_t start = t_vals[g - 1] + 1;
            size_t end = t_vals[g] < m ? t_vals[g] : m;
            if (start > end) continue;
            for (size_t x = end; ; --x)
            {
                insertion_order.push_back(x);
                if (x == start) break;
            }
        }

        // Helper: binary search lower_bound on result[0..pos) comparing values
        for (size_t ii = 0; ii < insertion_order.size(); ++ii)
        {
            size_t pid1based = insertion_order[ii];
            size_t pid = pid1based - 1; // zero-based index into pend_in_order and aligned sorted_main_chain

            size_t pend_idx = pend_in_order[pid]; // original index of pend value
            T pend_val = values[pend_idx];

            // associated main original index is at same relative position in sorted_main_chain
            size_t assoc_main_orig = sorted_main_chain[pid];

            // find assoc_main_orig in current result
            size_t pos = 0;
            for (; pos < result.size(); ++pos)
            {
                if (result[pos] == assoc_main_orig) break;
            }

            if (pos == 0)
            {
                // insert at front
                result.insert(result.begin(), pend_idx);
            }
            else
            {
                // binary search in [0, pos) using values[result[mid]] < pend_val
                size_t lo = 0;
                size_t hi = pos;
                while (lo < hi)
                {
                    size_t mid = (lo + hi) / 2;
                    if (values[ result[mid] ] < pend_val) lo = mid + 1; else hi = mid;
                }
                result.insert(result.begin() + lo, pend_idx);
            }
        }

        // Step 4: insert straggler if present using lower_bound on entire result
        if (has_straggler)
        {
            T sval = values[straggler_index];
            size_t lo = 0, hi = result.size();
            while (lo < hi)
            {
                size_t mid = (lo + hi) / 2;
                if (values[ result[mid] ] < sval) lo = mid + 1; else hi = mid;
            }
            result.insert(result.begin() + lo, straggler_index);
        }

        return result;
    }
};

#endif // PMERGEME_HPP
