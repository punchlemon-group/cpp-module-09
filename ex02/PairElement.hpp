#pragma once

#include <vector>
#include <utility>
#include <cstddef>
#include <algorithm>

#include "IElement.hpp"

template <typename Container>
class PairElement : public IElement<Container> {
public:
    typedef typename Container::value_type T;
    PairElement(IElement<Container>* elem1, IElement<Container>* elem2) {
        if (*elem1 > *elem2) {
            std::swap(elem1, elem2);
        }
        small_ = elem1;
        large_ = elem2;
    }

    PairElement(const PairElement& other) {
        small_ = other.small_->clone();
        large_ = other.large_->clone();
    }

    PairElement& operator=(const PairElement& other) {
        if (this != &other) {
            PairElement tmp(other);
            this->swap(tmp);
        }
        return *this;
    }

    ~PairElement() {
        delete small_;
        delete large_;
    }

    virtual typename IElement<Container>::ElementType getType() const {
        return IElement<Container>::TYPE_PAIR;
    }

    virtual T getValue() const {
        return large_->getValue();
    }

    virtual IElement<Container>* getLarge() const {
        return large_;
    }
    virtual IElement<Container>* getSmall() const {
        return small_;
    }

    virtual size_t getSize() const {
        return small_->getSize() + large_->getSize();
    }

    virtual void flatten(Container& vec) const {
        small_->flatten(vec);
        large_->flatten(vec);
    }

    virtual IElement<Container>* clone() const {
        return new PairElement(*this);
    }

    virtual void printElement() const {
        std::cout << "(";
        small_->printElement();
        std::cout << " ";
        large_->printElement();
        std::cout << ")";
    }

private:
    IElement<Container> *small_;
    IElement<Container> *large_;
    PairElement();

    void swap(PairElement &other) {
        std::swap(small_, other.small_);
        std::swap(large_, other.large_);
    }
};
