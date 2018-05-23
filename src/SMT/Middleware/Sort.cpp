#include "SMT/Middleware/Sort.h"

using namespace ceagle::smt;

SortType BoolSortImpl::type() const {
    return SortType::Bool;
}

SortType NumberSortImpl::type() const {
    return SortType::Number;
}

SortType IntSortImpl::type() const {
    return SortType::Int;
}

SortType BvSortImpl::type() const {
    return SortType::Bv;
}

SortType FloatSortImpl::type() const {
    return SortType::Float;
}

SortType RealSortImpl::type() const {
    return SortType::Real;
}

SortType ArraySortImpl::type() const {
    return SortType::Array;
}

void BoolSortImpl::print(std::ostream& out) const {
    out << "Bool";
}

void IntSortImpl::print(std::ostream& out) const {
    out << "Int";
}

void RealSortImpl::print(std::ostream& out) const {
    out << "Real";
}

void BvSortImpl::print(std::ostream& out) const {
    out << "(_ BitVec " << _bits << ")";
}

void FloatSortImpl::print(std::ostream& out) const {
    out << "(_ FloatingPoint " << eb << " " << sb << ")";
}

void ArraySortImpl::print(std::ostream& out) const {
    out << "(Array ";
    indexSort->print(out);
    out << " ";
    elementSort->print(out);
    out << ")";
}

SortImpl* BoolSortImpl::clone() const {
    return new BoolSortImpl();
}

SortImpl* IntSortImpl::clone() const {
    return new IntSortImpl();
}

SortImpl* RealSortImpl::clone() const {
    return new RealSortImpl();
}

SortImpl* BvSortImpl::clone() const {
    return new BvSortImpl(_bits);
}

SortImpl* FloatSortImpl::clone() const {
    return new FloatSortImpl(eb, sb);
}

SortImpl* ArraySortImpl::clone() const {
    return new ArraySortImpl(elementSort.get()->clone(), indexSort.get()->clone());
}

SortImpl* ArraySortImpl::getElementSort() const {
    return elementSort.get();
}
