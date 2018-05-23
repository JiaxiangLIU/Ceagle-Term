#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include "DFS/Advisor/AbsrefAdvisor.h"
#include "Util/IRUtil.h"
#include "debug.h"

using namespace llvm;

namespace {
    void debug_pause() {
        std::cin.get();
    }
}

namespace ceagle {

void AbsrefAdvisor::dfsVisit(const Module& module, const std::vector<const BasicBlock*>& path) {
    // since backward forbidden will revisit this block, we need to skip
    if (revisit) {
        revisit = false;
        return;
    }
    auto newLocation = path.back();
    outs() << "visit " << newLocation->getName() << "\n";
    if (argManager->empty()) {
        argManager->init(newLocation);
        return;
    }
    auto newArgNode = argManager->expand(module, newLocation);
    if (newArgNode->isEmpty()) {
        return;
    }
    if (isError(newLocation)) {
        outs() << "find potentioal error path\n";
        for (auto& bb: path) {
            outs() << bb->getName() << " ";
        }
        outs() << "\n";
        if (argManager->reachable(module)) {
            errorSet.insert(newLocation);
        } else {
            argManager->refine(module);
        }
    }

}

NextChoice<const BasicBlock*> AbsrefAdvisor::dfsNext(const Module& module, const std::vector<const BasicBlock*>& path) {
    if (errorSet.size() > 0) {
        return {NextPolicy::NP_BREAK_UNSAFE, {}};
    }
    return {NextPolicy::NP_CONTINUE_DETERMINED_ADVICE, argManager->next()};
}

BackwardPolicy AbsrefAdvisor::dfsBackward(const Module& module, const std::vector<const BasicBlock*>& path) {
    if (argManager->complete()) {
        outs() << "pop " << path.back()->getName();
        argManager->backward();
        return BackwardPolicy::BP_ALLOW;
    } else {
        revisit = true;
        outs() << "revisit " << path.back()->getName();
        return BackwardPolicy::BP_FORBIDDEN;
    }
}

//temp
bool AbsrefAdvisor::isError(const BasicBlock* block) const {
    auto name = block->getName().str();
    if (name.find("error") != std::string::npos || name.find("ERROR") != std::string::npos) {
        return true;
    }
    return false;
}

}
