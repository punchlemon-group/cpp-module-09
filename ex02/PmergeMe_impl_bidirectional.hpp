#include <list>
#include <cstddef>
#include <iterator>

// Bidirectional (list) implementation details for PmergeMe

// Bidirectional helper: build pairs using list values (no indices)
template <typename Container>
void createPairs(Container &c,
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

        typedef typename Container::value_type T;
        T v1 = *it;
        T v2 = *it2;
        T low = (v1 <= v2) ? v1 : v2;
        T high = (v1 <= v2) ? v2 : v1;
        out_pairs.push_back(std::make_pair(low, high));
        out_pendings.push_back(low);
        out_mains.push_back(high);

        ++it; ++it;
    }
}

// binarySearchAndInsert: find insertion position in [list.begin(), end_it) and insert value
template <typename Elem>
void binarySearchAndInsert(std::list<Elem> &lst, const Elem &value, typename std::list<Elem>::iterator end_it)
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
static inline std::list<std::ptrdiff_t> generateInsertionOrder(std::ptrdiff_t pend_count)
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
std::list<Elem> reorderPendings(const std::list<Elem> &sorted_main_chain, std::list< std::pair<Elem,Elem> > &pairs)
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

// insertPendingsInOrder: perform insertion loop (Nth access, find assoc main, binary insert)
template <typename Elem>
void insertPendingsInOrder(
    std::list<Elem> &result,
    const std::list<Elem> &pend_in_order,
    const std::list<Elem> &main_chain,
    const std::list<std::ptrdiff_t> &insertion_order)
{
    typedef std::ptrdiff_t idx_t;
    for (typename std::list<idx_t>::const_iterator pit = insertion_order.begin(); pit != insertion_order.end(); ++pit)
    {
        idx_t pid1 = *pit;
        idx_t pid = pid1 - 1;

        typename std::list<Elem>::const_iterator pend_it = pend_in_order.begin();
        for (idx_t s = 0; s < pid; ++s) ++pend_it;
        Elem pend_val = *pend_it;

        typename std::list<Elem>::const_iterator mains_it = main_chain.begin();
        for (idx_t s = 0; s < pid; ++s) ++mains_it;
        Elem assoc_main = *mains_it;

        typename std::list<Elem>::iterator assoc_it = result.begin();
        for (; assoc_it != result.end(); ++assoc_it) if (*assoc_it == assoc_main) break;

        if (assoc_it == result.begin()) { result.push_front(pend_val); continue; }

        binarySearchAndInsert<Elem>(result, pend_val, assoc_it);
    }
}

// Bidirectional sort_impl
template <typename Container>
void PmergeMe::sort_impl(Container &c, std::bidirectional_iterator_tag)
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
    std::list<Elem> pend_in_order = reorderPendings<Elem>(main_chain, pairs);

    // Build result list starting with sorted mains
    std::list<Elem> result;
    for (typename std::list<Elem>::iterator mit = main_chain.begin(); mit != main_chain.end(); ++mit) result.push_back(*mit);

    if (pend_in_order.empty()) { c.clear(); for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit); if (had_straggler) c.push_back(straggler_val); return; }

    // Insert first pending
    result.push_front(pend_in_order.front());

    typedef std::ptrdiff_t idx_t;
    idx_t pend_count = static_cast<idx_t>(std::distance(pend_in_order.begin(), pend_in_order.end()));
    std::list<idx_t> insertion_order = generateInsertionOrder(pend_count);

    // Insert pending elements according to insertion_order
    insertPendingsInOrder<Elem>(result, pend_in_order, main_chain, insertion_order);

    if (had_straggler)
        binarySearchAndInsert<Elem>(result, straggler_val, result.end());

    c.clear();
    for (typename std::list<Elem>::iterator rit = result.begin(); rit != result.end(); ++rit) c.push_back(*rit);
}
