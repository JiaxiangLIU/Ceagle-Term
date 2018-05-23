#ifndef CEAGLE_ABSREF_ABSTRACTOR_H
#define CEAGLE_ABSREF_ABSTRACTOR_H

#include "DFS/Absref/AbstractState.h"
#include "DFS/Absref/AbstractReachedSet.h"
#include "DFS/Absref/Precision.h"
#include "SMT/Middleware/Solver.h"

#include <llvm/IR/BasicBlock.h>

#include <memory>
#include <vector>

namespace ceagle {

class AbstractState;
class AbstractReachedSet;
class Precision;

class PredicateAbstractor {
    Precision p;
public:
    PredicateAbstractor() {}
    PredicateAbstractor(Precision& _p);
    AbstractState* abstract(AbstractState & start, std::map<std::string, Expr > valueMap = {}, Expr condition = BoolValue(true), llvm::BasicBlock* nextLocation = nullptr);

    void refine(std::vector<std::shared_ptr<AbstractState>> path);
    std::vector<Expr> refine(std::map<std::string, Expr> vm1, Expr c1, Expr c2);
    Precision& getPrecision() { return p; }

    std::vector<std::shared_ptr<AbstractState>> next(const std::shared_ptr<AbstractState> &state);
};


}

#endif
