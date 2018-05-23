#ifndef CEAGLE_ABSREF_PRECISION_H
#define CEAGLE_ABSREF_PRECISION_H

#include "SMT/Middleware/AST.h"

#include <llvm/IR/BasicBlock.h>

#include <map>
#include <unordered_set>

namespace ceagle {

using namespace smt;

class Precision {
    std::unordered_set<Expr> precision;
    std::vector<Expr> ordered;
    std::map<llvm::BasicBlock*, std::set<size_t>> locationPrecMap;
public:
    Precision() = default;
    Precision(std::unordered_set<Expr> p) : precision(p) {
        for (auto pre: precision) {
            ordered.push_back(pre);
        }
    }
    Precision(std::vector<Expr> p) {
        for (auto pre: p) {
            auto result = precision.insert(pre);
            if (result.second) {
                ordered.push_back(*result.first);
            }
        }
    }
    size_t size() const {
        return ordered.size();
    }
    Expr& get(size_t index) {
        return ordered[index-1];
    }
    size_t add(Expr prec) {
        auto result = precision.insert(prec);
        if (result.second) {
            ordered.push_back(*result.first);
            return ordered.size();
        }
        auto str = prec.str();
        return 1 + std::distance(ordered.begin(), std::find_if(ordered.begin(), ordered.end(), [&str](const Expr& e) {return str == e.str();}));
    }
    void merge(const Precision& p) {
    }
    std::set<size_t> getPrecisionForLocation(llvm::BasicBlock* location) {
        return locationPrecMap[location];
    }
    void addPrecision(llvm::BasicBlock* location, size_t p) {
        locationPrecMap[location].insert(p);
    }
};

}

#endif
