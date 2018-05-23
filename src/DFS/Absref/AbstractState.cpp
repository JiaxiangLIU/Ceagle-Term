#include "DFS/Absref/AbstractState.h"
#include "Util/IRUtil.h"
#include "SMT/Translator/InstTranslator.h"
#include "PredicateAbstractor.h"

#include <llvm/IR/Module.h>

using namespace llvm;

namespace ceagle {

using namespace util;

bool AbstractState::sound = false;

std::string AbstractState::outputElement(int i) const {
    if (i > 0) {
        return "b" + std::to_string(i);
    } else if (i < 0) {
        return "!b" + std::to_string(-i);
    } else {
        assert("predicate index should not be 0" && false);
    }
}

Expr AbstractState::intToExpr(int i) const {
    if (i > 0) {
        return Var("b!" + std::to_string(i), BoolSort());
    } else if (i < 0) {
        return !Var("b!" + std::to_string(-i), BoolSort());
    } else {
        assert("predicate index should not be 0" && false);
    }
}

std::ostream& operator<<(std::ostream& os, const AbstractState& obj) {
    if (obj._state.empty()) {
        os << "true";
    } else {
        auto it = obj._state.begin();
        auto et = obj._state.end();
        os << obj.outputElement(*it);
        while (++it != et) {
            os << " && " << obj.outputElement(*it);
        }
    }
    return os;
}

std::set<int> AbstractState::getPrecision() const {
    std::set<int> result;
    for (auto i : _state) {
        if (i > 0) {
            result.insert(i);
        } else {
            result.insert(-i);
        }
    }
    return result;
}

llvm::BasicBlock* AbstractState::location() const {
    return _location;
}

bool AbstractState::cover(const AbstractState& other) const {
    if (_location != other._location) return false;
    if (_phiSelector != other._phiSelector) return false;
    if (sound) {
        if (std::includes(other._state.begin(), other._state.end(), _state.begin(), _state.end())) return true;
    } else {
        if (std::includes(_state.begin(), _state.end(), other._state.begin(), other._state.end())) return true;
    }
    return false;
}

}
