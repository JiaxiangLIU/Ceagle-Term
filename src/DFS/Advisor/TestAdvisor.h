#ifndef CEAGLE_TEST_ADVISOR_H
#define CEAGLE_TEST_ADVISOR_H

#include "DFS/Advisor/Advisor.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h>

#include <map>

namespace ceagle {

class TestAdvisor : public Advisor<> {
    enum class Status {
        NEW = 0,
        VISITED,
        REVISITED
    };
    std::map<const llvm::BasicBlock*, Status> visited;
public:
    virtual void dfsVisit(const llvm::Module&, const std::vector<const llvm::BasicBlock*>&) override;
    virtual NextChoice<const llvm::BasicBlock*> dfsNext(const Module&, const std::vector<const BasicBlock*>&) override;
    virtual BackwardPolicy dfsBackward(const Module&, const std::vector<const BasicBlock*>&) override;
};

}

#endif
