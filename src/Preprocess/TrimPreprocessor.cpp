#include "Preprocess/TrimPreprocessor.h"
#include "Util/IRUtil.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <algorithm>
#include <iterator>
#include <set>

#define TRIM_LABEL "TRIM_LABEL"

namespace ceagle {

class TrimPreprocessor::TrimPreprocessorImpl : public ceagle::Preprocessor {
    std::vector<std::string> _targets;
    llvm::Function& getMainFunction(llvm::Module& module) {
        for (auto & F : module) {
            if (F.getName() != "main") continue;
            return F;
        }
        throw "No main function found when TrimPreprocessor preprocess module";
    }
    std::vector<llvm::BasicBlock*> findTargetsBlocks(llvm::Function* F) {
        auto& entry = F->front();
        std::map<llvm::BasicBlock*, bool> visited;
        std::vector<llvm::BasicBlock*> result;
        std::vector<llvm::BasicBlock*> waitList{&entry};
        while (waitList.size() > 0) {
            auto current = waitList.back();
            waitList.pop_back();
            visited[current] = true;
            if (belongsToTargets(current)) {
                result.push_back(current);
            }
            auto terminator = current->getTerminator();
            if (terminator != nullptr) {
                for (auto i = 0u, num = terminator->getNumSuccessors(); i < num; i++) {
                    auto successor = terminator->getSuccessor(i);
                    if (visited[successor]) continue;
                    waitList.push_back(successor);
                }
            }
        }
        return result;
    }
    std::set<llvm::BasicBlock*> findReachableBlocks(std::vector<llvm::BasicBlock*> targets) {
        std::set<llvm::BasicBlock*> result;
        result.insert(targets.begin(), targets.end());
        while ( targets.size() > 0) {
            auto current = targets.back();
            targets.pop_back();
            for (auto it = llvm::pred_begin(current), et = llvm::pred_end(current); it != et; ++it) {
                auto insertResult = result.insert(*it);
                if ( insertResult.second ) {
                    targets.push_back(*it);
                }
            }
        }
        return result;
    }
    std::vector<llvm::BasicBlock*> findUnreacableSet(llvm::Function& func, const std::set<llvm::BasicBlock*> & reachableSet) {
        std::set<llvm::BasicBlock*> blockList;
        for (auto & block: func) {
            blockList.insert(&block);
        }
        std::vector<llvm::BasicBlock*> result;
        std::set_difference(blockList.begin(), blockList.end(), reachableSet.begin(), reachableSet.end(), std::inserter(result, result.begin()));
        return result;
    }
    void removeBlocks(std::vector<llvm::BasicBlock*>& blocks, llvm::BasicBlock* trimReplaceBlock) {
        std::map<llvm::BasicBlock*, bool> needRemove;
        for (auto& bb: blocks) {
            needRemove[bb] = true;
        }
        for (auto& bb : blocks) {
            for (auto it = llvm::pred_begin(bb), et = llvm::pred_end(bb); it != et; ++it) {
                if (needRemove[*it]) continue;
                auto terminator = (*it)->getTerminator();
                switch (terminator->getOpcode()) {
                    // br, switch
                    case 2:case 3: {
                        // it must have at least 2 successor
                        assert(terminator->getNumSuccessors() >= 2);
                        // point the removed successor to assume fail
                        for ( auto i = 0u, num = terminator->getNumSuccessors(); i < num; i++ ) {
                            if (terminator->getSuccessor(i) == bb) {
                                terminator->setSuccessor(i, trimReplaceBlock);
                            }
                        }
                    }
                }
            }
            bb->eraseFromParent();
        }
    }
    bool belongsToTargets(const llvm::BasicBlock* block) {
        for (auto& target: _targets) {
            if (block->getName() == target) {
                return true;
            }
        }
        return false;
    }
    llvm::BasicBlock* createTrimReplaceBlock(llvm::LLVMContext& context, llvm::Function& mainFunction) {
        auto block = llvm::BasicBlock::Create(context, TRIM_LABEL, &mainFunction);
        if (mainFunction.getReturnType()->isVoidTy()) {
            llvm::ReturnInst::Create(context, block);
        } else {
            llvm::ReturnInst::Create(context, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), -1, true), block);
        }
        return block;
    }
public:
    TrimPreprocessorImpl(std::vector<std::string> targets) : _targets(targets) {
    }
    void preprocess(llvm::Module& module) {
        auto& mainFunction = getMainFunction(module);
        auto targets = findTargetsBlocks(&mainFunction);
        auto reachableSet = findReachableBlocks(targets);
        auto unreachableSet = findUnreacableSet(mainFunction, reachableSet);
        auto trimReplaceBlock = createTrimReplaceBlock(llvm::getGlobalContext(), mainFunction);
        removeBlocks(unreachableSet, trimReplaceBlock);
    }
};

}

namespace ceagle {

TrimPreprocessor::TrimPreprocessor(std::vector<std::string> targets) {
    impl.reset(new TrimPreprocessorImpl(targets));
}

void TrimPreprocessor::preprocess(llvm::Module& module) {
    impl->preprocess(module);
}

TrimPreprocessor::~TrimPreprocessor() = default;

TrimPreprocessor::TrimPreprocessor(TrimPreprocessor&& other) : impl(std::move(other.impl)) { }

}
