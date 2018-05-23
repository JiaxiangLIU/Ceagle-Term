#define DEFAULT_VERIFIER_ERROR_LABEL "SVCOMP_ERROR"
#define DEFAULT_VERIFIER_ASSUME_LABEL "SVCOMP_ASSUME_FAIL"
#ifndef CEAGLE_CEGAR_H
#define CEAGLE_CEGAR_H

#include "DFS/Absref/PredicateAbstractor.h"
#include "DFS/Absref/AbstractState.h"
#include "DFS/Absref/AbstractReachedSet.h"
#include "DFS/Absref/Precision.h"
#include "DFS/DFS.h"
#include "SMT/Middleware/AST.h"
#include "SMT/Middleware/Solver.h"
#include "SMT/Middleware/Z3RawSolver.h"

#include "SMT/Translator/InstTranslator.h"

#include <functional>
#include <vector>
#include <memory>

#define DEBUG_CEGAR 0

namespace ceagle {

using smt::CheckResult;
using smt::Z3RawSolver;

class CEGAR {
    PredicateAbstractor abstractor;
    AbstractReachedSet reached;
    std::vector<std::shared_ptr<AbstractState>> errorPath;
    std::map<llvm::BasicBlock*, Expr> witness;
    llvm::Module* module;
    bool unknownErrorPathFound = false;
protected:
    llvm::BasicBlock* entry(llvm::Module& module) const;
    bool isTarget(const std::shared_ptr<AbstractState>& state) const;
    bool isNewPath(const std::vector<std::shared_ptr<AbstractState>>&) const;
    void refine(std::vector<std::shared_ptr<AbstractState>> path);
    bool isFeasible(std::vector<std::shared_ptr<AbstractState>> path);
    void reportSafe();
    void reportError();
    DFSResult result;
    Model model;
public:
    CEGAR() { }
    virtual ~CEGAR() = default;
    virtual DFSResult getResult();
    virtual Model getModel();
    virtual decltype(witness) getWitness();
    std::vector<std::shared_ptr<AbstractState>> getErrorPath();
    virtual void visit(llvm::Module& module);

};

}

#endif
