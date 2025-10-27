#pragma once

#include <iostream>

template <typename Container>
class IElement {
public:
    typedef typename Container::value_type T;
    virtual ~IElement() {}

    enum ElementType {
        TYPE_SIZE_T,
        TYPE_PAIR
    };

    bool operator<(const IElement<Container>& other) const {
        return this->getValue() < other.getValue();
    }

    bool operator>(const IElement<Container>& other) const {
        return other < *this;
    }

    bool operator<=(const IElement<Container>& other) const {
        return !(other < *this);
    }

    bool operator>=(const IElement<Container>& other) const {
        return !(*this < other);
    }

    virtual ElementType getType() const = 0;
    virtual T getValue() const = 0;
    virtual size_t getSize() const = 0;
    virtual void flatten(Container& vec) const = 0;
    virtual IElement<Container>* clone() const = 0;
    virtual void printElement() const = 0;

protected:
    IElement() {}

private:
    IElement(const IElement<T>&);
    IElement<Container>& operator=(const IElement&);
};
