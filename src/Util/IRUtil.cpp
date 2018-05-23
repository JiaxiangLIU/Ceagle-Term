#include "Util/IRUtil.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <vector>
#include <utility>

using namespace llvm;

namespace {
    struct VariableRecorder : public InstVisitor<VariableRecorder> {
        std::vector<Value*> variables;

#define handle(instname) \
        void visit##instname(instname &I) { \
            variables.push_back(&I); \
        }

        handle(AllocaInst)
        handle(ICmpInst)
        handle(FCmpInst)
        handle(LoadInst)
        void visitStoreInst(StoreInst &I) {
            variables.push_back(I.getPointerOperand());
        }
        handle(TruncInst)
        handle(ZExtInst)
        handle(SExtInst)
        handle(FPTruncInst)
        handle(FPExtInst)
        handle(FPToUIInst)
        handle(FPToSIInst)
        handle(UIToFPInst)
        handle(SIToFPInst)
        handle(PtrToIntInst)
        handle(IntToPtrInst)
        handle(BitCastInst)
        //handle(CallInst)
        handle(BinaryOperator)
        handle(CmpInst)

    };
}

namespace ceagle {
namespace util {

std::vector<Value*> getChangedVariables(const BasicBlock* block) {
    VariableRecorder recorder;
    recorder.visit(const_cast<BasicBlock *>(block));
    return recorder.variables;
}

std::vector<std::pair<int, llvm::BasicBlock*>> getSuccessorsWithPhiSelector(llvm::BasicBlock* block) {
    std::vector<std::pair<int, llvm::BasicBlock*>> result;
    auto successors = getSuccessors(block);
    for (auto successor: successors) {
        auto firstInst = &(*(successor->begin()));
        auto opCode = firstInst->getOpcode();
        int phiSelector = -1;
        if (opCode == Instruction::PHI) {
            auto phi = dynamic_cast<PHINode*>(firstInst);
            size_t i, num;
            for(i = 0, num = phi->getNumIncomingValues(); i < num; i++) {
                auto pred = phi->getIncomingBlock(i);
                if (pred == block) {
                    phiSelector = i;
                    break;
                }
            }
            assert(i < num);
        }
        result.push_back({phiSelector, successor});
    }
    return result;
}

}
}
