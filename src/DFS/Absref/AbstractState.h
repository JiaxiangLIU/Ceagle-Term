#ifndef CEAGLE_ABSREF_ABSTRACTSTATE_H
#define CEAGLE_ABSREF_ABSTRACTSTATE_H

#include<set>

#include <llvm/IR/BasicBlock.h>

#include "SMT/Middleware/AST.h"

#include "DFS/Absref/PredicateAbstractor.h"

namespace ceagle {

using namespace smt;

class PredicateAbstractor;

class AbstractState {
    std::set<int> _state;
    std::string outputElement(int i) const;
    Expr intToExpr(int i) const;
public:
    static bool sound;
    AbstractState() { }
    AbstractState(std::set<int> state) : _state(state) { }
    AbstractState(llvm::BasicBlock* location) : _location(location), _state() { }
    AbstractState(llvm::BasicBlock* location, std::set<int> state) : _location(location), _state(state) { }
    friend std::ostream& operator<<(std::ostream& os, const AbstractState& obj);
    std::set<int> getPrecision() const;
    bool operator==(const AbstractState& other) const {
        return (_state == other._state) && (_location == other._location) && (_phiSelector == other._phiSelector);
    }
    operator Expr() {
        if (_state.empty()) return BoolValue(true);
        auto it = _state.begin();
        auto et = _state.end();
        Expr result = intToExpr(*it);
        while(++it != et) {
            result = result && intToExpr(*it);
        }
        return result;
    }
    llvm::BasicBlock* location() const;
    std::string name() const {
        return location()->getName();
    }
    void setPhiSelector(int i) { _phiSelector = i; }
    int getPhiSelector() { return _phiSelector; }
    bool cover(const AbstractState& other) const;

    llvm::BasicBlock* _location = nullptr;
    int _phiSelector = -1;
};


}

#endif
