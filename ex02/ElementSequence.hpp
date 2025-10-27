#pragma once

#include <vector>
#include <deque>
#include <cstddef>
#include <algorithm>
#include "IElement.hpp"
#include "SizeElement.hpp"
#include "PairElement.hpp"

template <typename Container>
struct StorageTypeTrait {
    typedef std::vector<IElement<Container>*> VectorStorage;
    typedef std::deque<IElement<Container>*> DequeStorage;
};

template <typename Container>
struct StorageSelector;

template <typename T, typename Alloc>
struct StorageSelector< std::vector<T, Alloc> > {
    typedef typename StorageTypeTrait< std::vector<T, Alloc> >::VectorStorage Type;
};

template <typename T, typename Alloc>
struct StorageSelector< std::deque<T, Alloc> > {
    typedef typename StorageTypeTrait< std::deque<T, Alloc> >::DequeStorage Type;
};

template <typename Container>
class ElementSequence {
public:
    typedef typename Container::value_type T;
    typedef typename StorageSelector<Container>::Type StorageContainer;

    explicit ElementSequence(const Container& values) {
        for (size_t i = 0; i < values.size(); ++i) {
            elements_.push_back(new SizeElement<Container>(values[i]));
        }
    }

    ElementSequence(const ElementSequence& other) {
        for (size_t i = 0; i < other.elements_.size(); ++i) {
            elements_.push_back(other.elements_[i]->clone());
        }
    }

    ElementSequence& operator=(ElementSequence other) {
        this->swap(other);
        return *this;
    }

    ~ElementSequence() {
        for (size_t i = 0; i < elements_.size(); ++i) {
            delete elements_[i];
        }
        elements_.clear();
    }

    void swap(ElementSequence& other) {
        elements_.swap(other.elements_);
    }

    Container getResult() const {
        Container result;
        for (size_t i = 0; i < this->elements_.size(); ++i) {
            this->elements_[i]->flatten(result);
        }
        return result;
    }

    void sort() {
        size_t size_level = this->elements_[0]->getSize();
        size_t i = 0;
        size_t count = 0;
        while (i < this->elements_.size()) {
            if (this->elements_[i]->getSize() == size_level) {
                ++count;
            }
            ++i;
        }
        if (count == 1) {
            return;
        }

        this->createPairs();
        this->sort();
        this->performInsertion();
    }

    void print() const {
        for (size_t i = 0; i < elements_.size(); ++i) {
            elements_[i]->printElement();
            if (i != elements_.size() - 1) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }

private:
    void createPairs() {
        size_t size_level = elements_[0]->getSize();
        StorageContainer paired_list;
        IElement<Container>* elem1 = NULL;
        IElement<Container>* elem2 = NULL;
        size_t i = 0;
        while (i < this->elements_.size()) {
            if (elem1) {
                elem2 = this->elements_[i];
            }
            else {
                elem1 = this->elements_[i];
            }
            if (elem1 && elem2 && elem1->getSize() == size_level && elem2->getSize() == size_level) {
                paired_list.push_back(new PairElement<Container>(elem1, elem2));
                elem1 = NULL;
                elem2 = NULL;
            }
            else if (i == this->elements_.size() - 1 || (elem1 && elem1->getSize() != size_level) || (elem2 && elem2->getSize() != size_level)) {
                if (elem1) {
                    paired_list.push_back(elem1);
                }
                if (elem2) {
                    paired_list.push_back(elem2);
                }
                for (size_t j = i + 1; j < this->elements_.size(); ++j) {
                    paired_list.push_back(this->elements_[j]);
                }
                break;
            }
            ++i;
        }
        this->elements_.swap(paired_list);
    }

    size_t binarySearch(IElement<Container>* element_to_insert, size_t index) {
        size_t lo = 0;
        size_t hi = index;
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            if (*(this->elements_[mid]) < *element_to_insert) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    void insertElement(size_t index, size_t size_level, size_t &last_index) {
        IElement<Container>* element_to_insert;
        if (this->elements_[index]->getSize() == size_level / 2) {
            element_to_insert = this->elements_[index];
            this->elements_.erase(this->elements_.begin() + index);
        }
        else {
            PairElement<Container>* pair_elem = dynamic_cast<PairElement<Container>*>(this->elements_[index]);
            if (!pair_elem) {
                std::cerr << "Error: insertElement called on non-PairElement" << std::endl; // will not happen
                exit(1);
            }
            element_to_insert = pair_elem->getSmall();
            this->elements_[index] = pair_elem->getLarge();
            ++last_index;
        }
        size_t insert_pos = this->binarySearch(element_to_insert, index);
        this->elements_.insert(this->elements_.begin() + insert_pos, element_to_insert);
    }

    size_t getLastIndex(size_t size_level) const {
        size_t last_index = 0;
        size_t i = elements_.size();
        while (i > 0) {
            --i;
            size_t size = elements_[i]->getSize();
            if (size == size_level || size == size_level / 2) {
                last_index = i;
                break;
            }
        }
        return last_index;
    }

    void performInsertion() {
        size_t size_level = elements_[0]->getSize();
        size_t last_index = getLastIndex(size_level);
        bool is_straggler = this->elements_[last_index]->getSize() == size_level / 2;
        insertElement(0, size_level, last_index);
        size_t i = 0;
        bool is_end = false;
        size_t two_pow = 4;
        while (!is_end) {
            i = two_pow - 1;
            if (i >= last_index) {
                i = last_index;
                is_end = true;
            }
            while (true) {
                if (!is_straggler || i != last_index || this->elements_[i]->getSize() != size_level / 2) {
                    while (elements_[i]->getSize() != size_level) {
                        if (i != 0)
                            --i;
                        else
                            break;
                    }
                    if (i == 0) {
                        if (is_straggler && is_end) {
                            i = last_index;
                        }
                        else {
                            break;
                        }
                    }
                }
                if (is_straggler && i == last_index && this->elements_[i]->getSize() == size_level / 2) {
                    is_straggler = false;
                }
                insertElement(i, size_level, last_index);
            }
            two_pow <<= 1;
        }
    }

    StorageContainer elements_;

    ElementSequence();
};
