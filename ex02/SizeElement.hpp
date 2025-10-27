#pragma once
#include "IElement.hpp"

template <typename Container>
class SizeElement : public IElement<Container> {
public:
    typedef typename Container::value_type T;
    SizeElement(T val) : value_(val) {}

    SizeElement(const SizeElement& other): value_(other.value_) {}

    SizeElement& operator=(const SizeElement& other) {
        if (this != &other) {
            value_ = other.value_;
        }
        return *this;
    }

    virtual ~SizeElement() {}

    virtual typename IElement<Container>::ElementType getType() const {
        return IElement<Container>::TYPE_SIZE_T;
    }

    virtual T getValue() const {
        return value_;
    }

    virtual size_t getSize() const {
        return 1;
    }

    virtual void flatten(Container& vec) const {
        vec.push_back(value_);
    }

    virtual IElement<Container>* clone() const {
        return new SizeElement(*this);
    }

    virtual void printElement() const {
        std::cout << value_;
    }

private:
    SizeElement();
    T value_;
};
