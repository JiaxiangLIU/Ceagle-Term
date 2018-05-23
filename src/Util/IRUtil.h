#ifndef CEAGLE_IRUTIL_H
#define CEAGLE_IRUTIL_H
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h>
#include <vector>
#include <utility>
namespace ceagle {
namespace util {

inline std::vector<const llvm::BasicBlock*> getSuccessors(const llvm::BasicBlock* block) {
    std::vector<const llvm::BasicBlock*> result;
    auto terminator = block->getTerminator();
    for(auto i = 0; i < terminator->getNumSuccessors(); i++) {
        result.push_back(terminator->getSuccessor(i));
    }
    return result;
}

inline std::vector<llvm::BasicBlock*> getSuccessors(llvm::BasicBlock* block) {
    std::vector<llvm::BasicBlock*> result;
    auto terminator = block->getTerminator();
    for(auto i = 0; i < terminator->getNumSuccessors(); i++) {
        result.push_back(terminator->getSuccessor(i));
    }
    return result;
}

std::vector<std::pair<int, llvm::BasicBlock*>> getSuccessorsWithPhiSelector(llvm::BasicBlock* block);


// get stored variable and assignment variable
std::vector<llvm::Value*> getChangedVariables(const llvm::BasicBlock* block);

}
}
#endif
