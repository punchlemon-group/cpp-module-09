#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <cstddef>

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
    template <typename T>
    struct LessByValue {
        const std::vector<T> *vals;
        LessByValue(const std::vector<T> *v): vals(v) {}
        bool operator()(size_t a, size_t b) const { return (*vals)[a] < (*vals)[b]; }
    };

    // Random-access helper: build pairs using indices
    template <typename Container>
    static void createPairs(Container &c,
        std::vector< std::pair<size_t, size_t> > &out_pairs,
        std::vector<size_t> &out_pends,
        bool &out_has_straggler,
        size_t &out_straggler_idx)
    {
        out_pairs.clear(); out_pends.clear(); out_has_straggler = false; out_straggler_idx = 0;
        size_t n = c.size();
        if (n == 0) return;

        size_t i = 0;
        for (; i + 1 < n; i += 2)
        {
            size_t a = i;
            size_t b = i + 1;
            typename Container::value_type &av = c[a];
            typename Container::value_type &bv = c[b];
            bool le = (av <= bv);
            size_t first_idx = le ? a : b;
            size_t second_idx = le ? b : a;
            out_pairs.push_back(std::make_pair(first_idx, second_idx));
            out_pends.push_back(first_idx);
        }
        if (i < n) { out_has_straggler = true; out_straggler_idx = i; }
    }

    // Bidirectional helper: build pairs using list values (no indices)
    template <typename Container>
    static void createPairs(Container &c,
        std::list< std::pair<typename Container::value_type, typename Container::value_type> > &out_pairs,
        std::list<typename Container::value_type> &out_pendings,
        std::list<typename Container::value_type> &out_mains,
        bool &out_has_straggler,
        typename Container::value_type &out_straggler_val)
    {
        out_pairs.clear(); out_pendings.clear(); out_mains.clear(); out_has_straggler = false;
        typename Container::iterator it = c.begin();
        while (it != c.end())
        {
            typename Container::iterator it2 = it; ++it2;
            if (it2 == c.end()) { out_has_straggler = true; out_straggler_val = *it; break; }

            typename Container::value_type a = *it;
            typename Container::value_type b = *it2;
            if (a <= b) { out_pairs.push_back(std::make_pair(a,b)); out_pendings.push_back(a); out_mains.push_back(b); }
            else { out_pairs.push_back(std::make_pair(b,a)); out_pendings.push_back(b); out_mains.push_back(a); }

            ++it; ++it;
        }
    }

    // Random-access implementation
    template <typename Container>
    void sort_impl(Container &c, std::random_access_iterator_tag)
    {
        std::vector< std::pair<size_t, size_t> > pairs;
        std::vector<size_t> pend_indices;
        bool has_straggler = false;
        size_t straggler_index = 0;
        createPairs<Container>(c, pairs, pend_indices, has_straggler, straggler_index);

        std::vector<size_t> sorted_main_chain;
        std::vector<size_t> pend_in_order;
        PmergeMe::template recursivelySortMainChain<Container>(c, pairs, pend_indices, sorted_main_chain, pend_in_order);

        if (pend_in_order.empty())
        {
            Container tmp;
            for (size_t r = 0; r < sorted_main_chain.size(); ++r) tmp.push_back(c[ sorted_main_chain[r] ]);
            if (has_straggler) tmp.push_back(c[ straggler_index ]);
            c.swap(tmp);
            return;
        }

        std::vector<size_t> result_idx = sorted_main_chain;
        result_idx.insert(result_idx.begin(), pend_in_order[0]);

        std::vector<size_t> insertion_order = PmergeMe::generateInsertionOrder(pend_in_order.size());

        PmergeMe::template insertPendings<Container>(c, result_idx, pend_in_order, sorted_main_chain, insertion_order);
        if (has_straggler)
            PmergeMe::template insertStragglerIntoResult<Container>(c, result_idx, straggler_index);

        PmergeMe::template applyResult<Container>(c, result_idx);
    }

    // Bidirectional implementation
    template <typename Container>
    void sort_impl(Container &c, std::bidirectional_iterator_tag)
    {
        typedef typename Container::value_type Elem;
        if (c.size() <= 1) return;

        std::list< std::pair<Elem, Elem> > pairs;
        std::list<Elem> pendings;
        std::list<Elem> main_chain;
        bool had_straggler = false;
        Elem straggler_val = Elem();
        createPairs<Container>(c, pairs, pendings, main_chain, had_straggler, straggler_val);

        // Recursively sort mains (list)
        sort_impl(main_chain, std::bidirectional_iterator_tag());
        // Rebuild pendings in the order corresponding to the sorted main_chain.
        std::list<Elem> pend_in_order = PmergeMe::template reorderPendings<Elem>(main_chain, pairs);

        // Build result list starting with sorted mains
        std::list<Elem> result;
        for (typename std::list<Elem>::iterator mit = main_chain.begin(); mit != main_chain.end(); ++mit) result.push_back(*mit);

        if (pend_in_order.empty()) { c.clear(); for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit); if (had_straggler) c.push_back(straggler_val); return; }

        // Insert first pending
        result.push_front(pend_in_order.front());

        typedef std::ptrdiff_t idx_t;
        idx_t pend_count = static_cast<idx_t>(std::distance(pend_in_order.begin(), pend_in_order.end()));
        std::list<idx_t> insertion_order = PmergeMe::generateInsertionOrder(pend_count);

        for (typename std::list<idx_t>::iterator pit = insertion_order.begin(); pit != insertion_order.end(); ++pit)
        {
            idx_t pid1 = *pit;
            idx_t pid = pid1 - 1;

            typename std::list<Elem>::iterator pend_it = pend_in_order.begin();
            for (idx_t s = 0; s < pid; ++s) ++pend_it;
            Elem pend_val = *pend_it;

            typename std::list<Elem>::iterator mains_it = main_chain.begin();
            for (idx_t s = 0; s < pid; ++s) ++mains_it;
            Elem assoc_main = *mains_it;

            typename std::list<Elem>::iterator assoc_it = result.begin();
            for (; assoc_it != result.end(); ++assoc_it) if (*assoc_it == assoc_main) break;

            if (assoc_it == result.begin()) { result.push_front(pend_val); continue; }

            PmergeMe::template binarySearchAndInsert<Elem>(result, pend_val, assoc_it);
        }

        if (had_straggler)
            PmergeMe::template binarySearchAndInsert<Elem>(result, straggler_val, result.end());

        c.clear();
        for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit);
    }

    template <typename T>
    static std::vector<size_t> fordJohnsonOrder(const std::vector<T> &values)
    {
        size_t n = values.size();
        if (n == 0) return std::vector<size_t>();
        if (n == 1) { std::vector<size_t> r; r.push_back(0); return r; }

        std::vector< std::pair<size_t, size_t> > pairs;
        std::vector<size_t> pend_indices;
        size_t i = 0;
        for (; i + 1 < n; i += 2)
        {
            size_t a = i, b = i + 1;
            if (values[a] <= values[b]) { pairs.push_back(std::make_pair(a,b)); pend_indices.push_back(a); }
            else { pairs.push_back(std::make_pair(b,a)); pend_indices.push_back(b); }
        }
        bool has_straggler = false; size_t straggler_index = 0;
        if (i < n) { has_straggler = true; straggler_index = i; }

        std::vector<size_t> main_chain_indices;
        for (size_t j = 0; j < pairs.size(); ++j) main_chain_indices.push_back(pairs[j].second);

        std::vector<T> main_values;
        for (size_t j = 0; j < main_chain_indices.size(); ++j) main_values.push_back(values[ main_chain_indices[j] ]);

        std::vector<size_t> mains_order = fordJohnsonOrder(main_values);

        std::vector<size_t> sorted_main_chain;
        std::vector<size_t> pend_in_order;
        for (size_t k = 0; k < mains_order.size(); ++k)
        {
            size_t idx_in_pairs = mains_order[k];
            sorted_main_chain.push_back(main_chain_indices[idx_in_pairs]);
            pend_in_order.push_back(pend_indices[idx_in_pairs]);
        }

        if (pend_in_order.empty()) return sorted_main_chain;

        std::vector<size_t> result = sorted_main_chain;
        result.insert(result.begin(), pend_in_order[0]);

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
            size_t start = t_vals[g-1] + 1;
            size_t end = t_vals[g] < m ? t_vals[g] : m;
            if (start > end) continue;
            for (size_t x = end; ; --x) { insertion_order.push_back(x); if (x == start) break; }
        }

        for (size_t ii = 0; ii < insertion_order.size(); ++ii)
        {
            size_t pid = insertion_order[ii] - 1;
            size_t pend_idx = pend_in_order[pid];
            T pend_val = values[pend_idx];
            size_t assoc_main_orig = sorted_main_chain[pid];

            size_t pos = 0; for (; pos < result.size(); ++pos) if (result[pos] == assoc_main_orig) break;
            if (pos == 0) { result.insert(result.begin(), pend_idx); }
            else {
                size_t lo = 0, hi = pos;
                while (lo < hi) {
                    size_t mid = (lo + hi) / 2;
                    if (values[ result[mid] ] < pend_val) lo = mid + 1; else hi = mid;
                }
                result.insert(result.begin() + lo, pend_idx);
            }
        }

        if (has_straggler)
        {
            T sval = values[straggler_index];
            size_t lo = 0, hi = result.size();
            while (lo < hi) {
                size_t mid = (lo + hi) / 2;
                if (values[ result[mid] ] < sval) lo = mid + 1; else hi = mid;
            }
            result.insert(result.begin() + lo, straggler_index);
        }

        return result;
    }

    // private helpers

    // Helper: recursively sort main chain and produce sorted_main_chain + pend_in_order
    template <typename Container>
    static void recursivelySortMainChain(
        Container &c,
        const std::vector< std::pair<size_t,size_t> > &pairs,
        const std::vector<size_t> &pend_indices,
        std::vector<size_t> &out_sorted_main_chain,
        std::vector<size_t> &out_pend_in_order)
    {
        out_sorted_main_chain.clear(); out_pend_in_order.clear();
        std::vector<size_t> main_chain_indices;
        for (size_t j = 0; j < pairs.size(); ++j) main_chain_indices.push_back(pairs[j].second);

        std::vector<typename Container::value_type> main_values;
        for (size_t j = 0; j < main_chain_indices.size(); ++j) main_values.push_back(c[ main_chain_indices[j] ]);

        std::vector<size_t> mains_order = fordJohnsonOrder(main_values);

        for (size_t k = 0; k < mains_order.size(); ++k)
        {
            size_t idx_in_pairs = mains_order[k];
            out_sorted_main_chain.push_back(main_chain_indices[idx_in_pairs]);
            out_pend_in_order.push_back(pend_indices[idx_in_pairs]);
        }
    }

    // Helper: generate insertion order from Jacobsthal-like sequence
    static std::vector<size_t> generateInsertionOrder(size_t pend_count)
    {
        std::vector<size_t> insertion_order;
        if (pend_count == 0) return insertion_order;
        std::vector<size_t> t_vals;
        for (size_t k = 1; ; ++k)
        {
            unsigned long pow2 = 1UL << (k + 1);
            int sign = (k % 2 == 0) ? 1 : -1;
            unsigned long t = (pow2 + (unsigned long)sign) / 3UL;
            t_vals.push_back((size_t)t);
            if (t >= pend_count) break;
        }
        for (size_t g = 1; g < t_vals.size(); ++g)
        {
            size_t start = t_vals[g-1] + 1;
            size_t end = t_vals[g] < pend_count ? t_vals[g] : pend_count;
            if (start > end) continue;
            for (size_t x = end; ; --x) {
                insertion_order.push_back(x);
                if (x == start) break;
            }
        }
        return insertion_order;
    }

    // Helper: insert pendings according to insertion_order
    template <typename Container>
    static void insertPendings(Container &c,
        std::vector<size_t> &result_idx,
        const std::vector<size_t> &pend_in_order,
        const std::vector<size_t> &sorted_main_chain,
        const std::vector<size_t> &insertion_order)
    {
        for (size_t ii = 0; ii < insertion_order.size(); ++ii)
        {
            size_t pid = insertion_order[ii] - 1;
            size_t pend_idx = pend_in_order[pid];
            typedef typename Container::value_type Elem;
            Elem pend_val = c[ pend_idx ];
            size_t assoc_main_orig = sorted_main_chain[pid];

            size_t pos = 0;
            for (; pos < result_idx.size(); ++pos) if (result_idx[pos] == assoc_main_orig) break;

            if (pos == 0) { result_idx.insert(result_idx.begin(), pend_idx); }
            else {
                size_t lo = 0, hi = pos;
                while (lo < hi) {
                    size_t mid = (lo + hi) / 2;
                    if (c[ result_idx[mid] ] < pend_val) lo = mid + 1; else hi = mid;
                }
                result_idx.insert(result_idx.begin() + lo, pend_idx);
            }
        }
    }

    // Helper: insert straggler into result_idx
    template <typename Container>
    static void insertStragglerIntoResult(Container &c, std::vector<size_t> &result_idx, size_t straggler_index)
    {
        typedef typename Container::value_type Elem;
        Elem sval = c[straggler_index];
        size_t lo = 0, hi = result_idx.size();
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            if (c[ result_idx[mid] ] < sval) lo = mid + 1; else hi = mid;
        }
        result_idx.insert(result_idx.begin() + lo, straggler_index);
    }

    // Helper: apply result_idx to container
    template <typename Container>
    static void applyResult(Container &c, const std::vector<size_t> &result_idx)
    {
        Container tmp;
        for (size_t r = 0; r < result_idx.size(); ++r) tmp.push_back(c[ result_idx[r] ]);
        c.swap(tmp);
    }

    // List-specific helpers (for bidirectional implementation)

    // binarySearchAndInsert: find insertion position in [list.begin(), end_it) and insert value
    template <typename Elem>
    static void binarySearchAndInsert(std::list<Elem> &lst, const Elem &value, typename std::list<Elem>::iterator end_it)
    {
        typedef std::ptrdiff_t idx_t;
        idx_t len = static_cast<idx_t>(std::distance(lst.begin(), end_it));
        idx_t lo = 0, hi = len;
        while (lo < hi) {
            idx_t mid = (lo + hi) / 2;
            typename std::list<Elem>::iterator midit = lst.begin();
            for (idx_t s = 0; s < mid; ++s) ++midit;
            if (*midit < value) lo = mid + 1; else hi = mid;
        }
        typename std::list<Elem>::iterator insert_it = lst.begin();
        for (idx_t s = 0; s < lo; ++s) ++insert_it;
        lst.insert(insert_it, value);
    }

    // generateInsertionOrder for list-based pendings (returns Jacobsthal-based order)
    static std::list<std::ptrdiff_t> generateInsertionOrder(std::ptrdiff_t pend_count)
    {
        std::list<std::ptrdiff_t> insertion_order;
        if (pend_count <= 0) return insertion_order;
        std::vector<std::ptrdiff_t> t_vals;
        for (std::ptrdiff_t k = 1; ; ++k)
        {
            unsigned long pow2 = 1UL << (k + 1);
            int sign = (k % 2 == 0) ? 1 : -1;
            unsigned long t = (pow2 + (unsigned long)sign) / 3UL;
            t_vals.push_back(static_cast<std::ptrdiff_t>(t));
            if (t >= static_cast<unsigned long>(pend_count)) break;
        }
        for (size_t g = 1; g < t_vals.size(); ++g)
        {
            std::ptrdiff_t start = t_vals[g-1] + 1;
            std::ptrdiff_t end = t_vals[g] < pend_count ? t_vals[g] : pend_count;
            if (start > end) continue;
            for (std::ptrdiff_t x = end; x >= start; --x) insertion_order.push_back(x);
        }
        return insertion_order;
    }

    // reorderPendings: reconstruct pend_in_order from sorted_main_chain and original pairs
    template <typename Elem>
    static std::list<Elem> reorderPendings(const std::list<Elem> &sorted_main_chain, std::list< std::pair<Elem,Elem> > &pairs)
    {
        std::list<Elem> pend_in_order;
        for (typename std::list<Elem>::const_iterator mit = sorted_main_chain.begin(); mit != sorted_main_chain.end(); ++mit)
        {
            typename std::list< std::pair<Elem,Elem> >::iterator pit = pairs.begin();
            for (; pit != pairs.end(); ++pit)
            {
                if (pit->second == *mit)
                {
                    pend_in_order.push_back(pit->first);
                    pairs.erase(pit);
                    break;
                }
            }
        }
        return pend_in_order;
    }
};

#endif // PMERGEME_HPP
