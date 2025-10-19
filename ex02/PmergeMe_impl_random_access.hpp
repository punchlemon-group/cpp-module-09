#include <vector>
#include <algorithm>
#include <iterator>
#include <cstddef>
#include <utility>

// Random-access (vector) implementation details for PmergeMe

// forward declaration so recursivelySortMainChain (which appears earlier) can call it
template <typename T> std::vector<size_t> fordJohnsonOrder(const std::vector<T> &values);

// LessByValue
template <typename T>
struct LessByValue {
    const std::vector<T> *vals;
    LessByValue(const std::vector<T> *v): vals(v) {}
    bool operator()(size_t a, size_t b) const { return (*vals)[a] < (*vals)[b]; }
};

// Reorderer: transform index -> element by looking up a source vector
template <typename T>
struct Reorderer
{
    const std::vector<T> &source_vec;

    Reorderer(const std::vector<T> &source) : source_vec(source) {}

    T operator()(size_t index) const
    {
        return source_vec[index];
    }
};

// Helper: binary search + insert for index-based result vector
template <typename T>
void binarySearchAndInsertIndex(
    std::vector<size_t> &result_idx,
    const std::vector<T> &values,
    size_t index_to_insert,
    size_t search_range_end)
{
    T value_to_insert = values[index_to_insert];
    size_t lo = 0, hi = search_range_end;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (values[ result_idx[mid] ] < value_to_insert) lo = mid + 1; else hi = mid;
    }
    result_idx.insert(result_idx.begin() + lo, index_to_insert);
}

// Random-access helper: build pairs using indices
template <typename Container>
static void createPairs(const Container &c,
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
        const typename Container::value_type &av = c[a];
        const typename Container::value_type &bv = c[b];
        bool le = (av <= bv);
        size_t first_idx = le ? a : b;
        size_t second_idx = le ? b : a;
        out_pairs.push_back(std::make_pair(first_idx, second_idx));
        out_pends.push_back(first_idx);
    }
    if (i < n) { out_has_straggler = true; out_straggler_idx = i; }
}

// Compare pairs by the main value (pair.first indexes into the source container)
template <typename Container>
struct ComparePairsByMainValue
{
    const Container &source_container;
    ComparePairsByMainValue(const Container &c) : source_container(c) {}
    bool operator()(const std::pair<size_t, size_t> &p1, const std::pair<size_t, size_t> &p2) const
    {
        return source_container[p1.first] < source_container[p2.first];
    }
};

// Recursively sort a vector of (main, pend) pairs in-place using the Ford-Johnson order
// applied to the main values. This helper reuses fordJohnsonOrder on the main values
// and then permutes the pairs accordingly.
// forward-declare generateInsertionOrder so templates can call it
static inline std::vector<size_t> generateInsertionOrder(size_t pend_count);

template <typename Container>
static std::vector<size_t> recursivelySortPairs(Container &c, std::vector<std::pair<size_t, size_t> > &pairs_to_sort, bool has_straggler, size_t straggler_index)
{
    std::vector<size_t> empty_result;
    if (pairs_to_sort.size() == 0) {
        // if there is only a straggler, return it
        if (has_straggler) {
            empty_result.push_back(straggler_index);
        }
        return empty_result;
    }

    if (pairs_to_sort.size() == 1)
    {
        std::vector<size_t> r;
        r.push_back(pairs_to_sort[0].first);
        r.push_back(pairs_to_sort[0].second);
        if (has_straggler) r.push_back(straggler_index);
        return r;
    }

    // copy container values into a vector for binary searches and comparisons
    typedef typename Container::value_type Elem;
    std::vector<Elem> values;
    values.reserve(c.size());
    for (size_t i = 0; i < c.size(); ++i) values.push_back(c[i]);

    // extract main values (values at pair.first)
    std::vector<Elem> main_values;
    main_values.reserve(pairs_to_sort.size());
    for (size_t i = 0; i < pairs_to_sort.size(); ++i)
        main_values.push_back(values[ pairs_to_sort[i].first ]);

    // compute Ford-Johnson order for main values
    std::vector<size_t> mains_order = fordJohnsonOrder(main_values);

    // build sorted_main_chain and pend_in_order according to mains_order
    std::vector<size_t> sorted_main_chain;
    std::vector<size_t> pend_in_order;
    sorted_main_chain.reserve(mains_order.size());
    pend_in_order.reserve(mains_order.size());
    for (size_t k = 0; k < mains_order.size(); ++k) {
        size_t idx_in_pairs = mains_order[k];
        sorted_main_chain.push_back(pairs_to_sort[idx_in_pairs].first);
        pend_in_order.push_back(pairs_to_sort[idx_in_pairs].second);
    }

    // if there are no pendings, result is just sorted mains (+ optional straggler)
    if (pend_in_order.empty()) {
        std::vector<size_t> result = sorted_main_chain;
        if (has_straggler) result.push_back(straggler_index);
        return result;
    }

    // perform Jacobsthal-based insertion of pendings into sorted_main_chain
    std::vector<size_t> result = sorted_main_chain;
    // insert the first pend at the beginning
    result.insert(result.begin(), pend_in_order[0]);

    size_t m = pend_in_order.size();
    std::vector<size_t> insertion_order = generateInsertionOrder(m);

    for (size_t ii = 0; ii < insertion_order.size(); ++ii)
    {
        size_t pid = insertion_order[ii] - 1;
        size_t pend_idx = pend_in_order[pid];
        size_t assoc_main_orig = sorted_main_chain[pid];

        size_t pos = 0; for (; pos < result.size(); ++pos) if (result[pos] == assoc_main_orig) break;
        if (pos == 0) { result.insert(result.begin(), pend_idx); }
        else {
            // binary search within [0, pos) and insert
            binarySearchAndInsertIndex<Elem>(result, values, pend_idx, pos);
        }
    }

    if (has_straggler)
        binarySearchAndInsertIndex<Elem>(result, values, straggler_index, result.size());

    return result;
}

// Helper: generate insertion order from Jacobsthal-like sequence (vector variant)
static inline std::vector<size_t> generateInsertionOrder(size_t pend_count)
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

// Helper: insert pendings according to insertion_order (vector/index-based)
template <typename Container>
void insertPendings(Container &c,
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
void insertStragglerIntoResult(Container &c, std::vector<size_t> &result_idx, size_t straggler_index)
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
void applyResult(Container &c, const std::vector<size_t> &result_idx)
{
    Container tmp;
    for (size_t r = 0; r < result_idx.size(); ++r) tmp.push_back(c[ result_idx[r] ]);
    c.swap(tmp);
}

// fordJohnsonOrder (index based)
template <typename T>
std::vector<size_t> fordJohnsonOrder(const std::vector<T> &values)
{
    size_t n = values.size();
    if (n == 0) return std::vector<size_t>();
    if (n == 1) { std::vector<size_t> r; r.push_back(0); return r; }

    std::vector< std::pair<size_t, size_t> > pairs;
    std::vector<size_t> pend_indices;
    bool has_straggler = false; size_t straggler_index = 0;
    // use the unified createPairs (explicitly instantiate with std::vector<T>)
    createPairs< std::vector<T> >(values, pairs, pend_indices, has_straggler, straggler_index);

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
    std::vector<size_t> insertion_order = generateInsertionOrder(m);

    for (size_t ii = 0; ii < insertion_order.size(); ++ii)
    {
        size_t pid = insertion_order[ii] - 1;
        size_t pend_idx = pend_in_order[pid];
        size_t assoc_main_orig = sorted_main_chain[pid];

        size_t pos = 0; for (; pos < result.size(); ++pos) if (result[pos] == assoc_main_orig) break;
        if (pos == 0) { result.insert(result.begin(), pend_idx); }
        else {
            // use helper to binary-search within [0, pos) and insert
            binarySearchAndInsertIndex<T>(result, values, pend_idx, pos);
        }
    }

    if (has_straggler)
        binarySearchAndInsertIndex<T>(result, values, straggler_index, result.size());

    return result;
}

// Random-access sort_impl
template <typename Container>
void PmergeMe::sort_impl(Container &c, std::random_access_iterator_tag)
{
    std::vector< std::pair<size_t, size_t> > pairs;
    std::vector<size_t> pend_indices;
    bool has_straggler = false;
    size_t straggler_index = 0;
    createPairs<Container>(c, pairs, pend_indices, has_straggler, straggler_index);

    // Build vector of (main, pend) pairs where first = main (bigger), second = pend (smaller)
    std::vector< std::pair<size_t, size_t> > main_pend_pairs;
    main_pend_pairs.reserve(pairs.size());
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        // pairs[i] already stores (smaller, larger) as (first, second) in createPairs,
        // but we want pair.first == main (larger) and pair.second == pend (smaller)
        size_t small_idx = pairs[i].first;
        size_t large_idx = pairs[i].second;
        main_pend_pairs.push_back(std::make_pair(large_idx, small_idx));
    }

    // Compute final index order using the Ford-Johnson based pair sorter
    std::vector<size_t> result_idx = recursivelySortPairs<Container>(c, main_pend_pairs, has_straggler, straggler_index);

    applyResult<Container>(c, result_idx);
}
