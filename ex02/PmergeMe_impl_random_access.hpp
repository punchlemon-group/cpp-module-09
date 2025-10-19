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

// Helper: create index-based pairs from a values vector (used by fordJohnsonOrder)
template <typename T>
void createPairsFromIndices(
    const std::vector<T> &values,
    std::vector< std::pair<size_t, size_t> > &out_pairs,
    std::vector<size_t> &out_pends,
    bool &out_has_straggler,
    size_t &out_straggler_idx)
{
    out_pairs.clear(); out_pends.clear(); out_has_straggler = false; out_straggler_idx = 0;
    size_t n = values.size();
    if (n == 0) return;
    size_t i = 0;
    for (; i + 1 < n; i += 2)
    {
        size_t a = i;
        size_t b = i + 1;
        if (values[a] <= values[b]) { out_pairs.push_back(std::make_pair(a,b)); out_pends.push_back(a); }
        else { out_pairs.push_back(std::make_pair(b,a)); out_pends.push_back(b); }
    }
    if (i < n) { out_has_straggler = true; out_straggler_idx = i; }
}

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
void createPairs(Container &c,
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

// Helper: recursively sort main chain and produce sorted_main_chain + pend_in_order
template <typename Container>
void recursivelySortMainChain(
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
    createPairsFromIndices<T>(values, pairs, pend_indices, has_straggler, straggler_index);

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

    std::vector<size_t> sorted_main_chain;
    std::vector<size_t> pend_in_order;
    recursivelySortMainChain<Container>(c, pairs, pend_indices, sorted_main_chain, pend_in_order);

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

    std::vector<size_t> insertion_order = generateInsertionOrder(pend_in_order.size());

    insertPendings<Container>(c, result_idx, pend_in_order, sorted_main_chain, insertion_order);
    if (has_straggler)
        insertStragglerIntoResult<Container>(c, result_idx, straggler_index);

    applyResult<Container>(c, result_idx);
}
