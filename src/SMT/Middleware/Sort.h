#ifndef CEAGLE_SMT_SORT_H
#define CEAGLE_SMT_SORT_H

#include <ostream>
#include <memory>

namespace ceagle {

namespace smt {

enum class SortType {
    None = 0,
    Bool,
    Number,
    Int,
    Bv,
    Float,
    Real,
    Array
};

class SortImpl {
    friend class Sort;
public:
    virtual void print(std::ostream& out) const { }
    virtual SortType type() const { return SortType::None; }
    virtual SortImpl* clone() const { return new SortImpl(); }
    virtual SortImpl* getElementSort() const { throw "not implemented"; }
public:
    virtual ~SortImpl() = default;
};

class BoolSortImpl : public SortImpl {
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
};

class NumberSortImpl : public SortImpl {
    virtual SortType type() const override;
protected:
    std::size_t _bits;
public:
    NumberSortImpl(std::size_t size) : _bits(size) {
    }
    std::size_t getSize() const { return _bits; }
};

class IntSortImpl : public NumberSortImpl {
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
public:
    IntSortImpl() : NumberSortImpl(32) {
    }
};

class RealSortImpl : public NumberSortImpl {
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
public:
    RealSortImpl() : NumberSortImpl(-1) {
    }
};

class BvSortImpl : public NumberSortImpl {
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
public:
    BvSortImpl(std::size_t size) : NumberSortImpl(size) {
    }
};

class FloatSortImpl : public NumberSortImpl {
protected:
    std::size_t eb;
    std::size_t sb;
private:
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
public:
    FloatSortImpl(std::size_t e, std::size_t s) : NumberSortImpl(e+s), eb(e), sb(s) {
    }
    inline std::size_t getEb() const { return eb; }
    inline std::size_t getSb() const { return sb; }
};

class ArraySortImpl : public SortImpl {
protected:
    std::unique_ptr<SortImpl> elementSort;
    std::unique_ptr<SortImpl> indexSort;
public:
    ArraySortImpl(SortImpl* elementSort, SortImpl* indexSort) {
        this->elementSort.reset(elementSort);
        this->indexSort.reset(indexSort);
    }
    virtual SortType type() const override;
    virtual void print(std::ostream& out) const override;
    virtual SortImpl* clone() const override;
    virtual SortImpl* getElementSort() const override;
};

}

}
#endif
