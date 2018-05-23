#include "ArraysBoundsAdvisor.h"

#include <iostream>
#include "llvm/IR/Instructions.h"
#include "llvm/Support/FormattedStream.h"

namespace ceagle {

void ArraysBoundsAdvisor::dfsVisit(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsVisit(module, currentPath);
    std::cout << "ArraysBoundsAdvisor::dfsVisit" << std::endl;

    llvm::Function* mainFunction = module.getFunction("main");
    for (llvm::Function::iterator fi = mainFunction->begin(), fe = mainFunction->end(); fi != fe; ++ fi) {
        for (BasicBlock::const_iterator bi = fi->begin(), be = fi->end(); bi != be; ++ bi) {
            if (const llvm::ReturnInst* inst = llvm::dyn_cast<llvm::ReturnInst>(bi)) {
                //llvm::outs() << "Found ReturnInst " << inst  << "\n";
                std::cout << "Found a ReturnInst" << std::endl;
            }
        }
    }
    return;

    //----

    // DW; check if this the end of the program
    auto currentNode = currentPath.back();
    for (BasicBlock::const_iterator bi = currentNode->begin(), be = currentNode->end(); bi != be; ++ bi) {
        if (const llvm::ReturnInst* inst = llvm::dyn_cast<llvm::ReturnInst>(bi)) {
            //llvm::outs() << "Found ReturnInst " << inst  << "\n";
            std::cout << "Found a ReturnInst" << std::endl;
        }
    }
    if (currentNode->back().isTerminator()) {
#if DEBUG_ADVISOR
        //llvm::outs() << "Found terminator " << static_cast<void*>(&(currentNode->back())) << "\n";
        llvm::outs() << "ArraysBoundsAdvisor currentNode back " << currentNode->back() << "\n";
#endif
    }
    auto terminator = currentNode->getTerminator();
#if DEBUG_ADVISOR
    //llvm::outs() << "Visiting " << static_cast<void*>(&currentNode) << ":" << currentNode << ", terminator " << static_cast<void*>(&terminator) << ":" << terminator << "\n";
#endif

    auto succCount = terminator->getNumSuccessors();
    std::cout << "ArraysBoundsAdvisor succCount:" << succCount << std::endl;
    std::vector<P_BB> choices;
    for(auto i = 0; i < succCount; ++i) {
        auto b = terminator->getSuccessor(i);

        // if b is an error node, it will be inserted first
        if (b->getName() == STR_LABEL_ERROR) {
            //std::cout << "Inserting error" << std::endl;
            //llvm::outs() << b->getName() << "\n";
            auto it = choices.begin();
            it = choices.insert(it, b);
        } else if (b->getName() == STR_LABEL_ASSUME_FAIL) {
#if NAIVEDEBUG
            std::cout << "ArraysBoundsAdvisor found an assume fail label, skipping it" << std::endl;
#endif
        } else {
            choices.push_back(b);
        }
    }
    this->m_nextChoice = {NextPolicy::NP_CONTINUE_NO_ADVICE, choices};
}

NextChoice<P_BB> ArraysBoundsAdvisor::dfsNext(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsNext(module, currentPath);
    std::cout << "ArraysBoundsAdvisor::dfsNext" << std::endl;
    return this->m_nextChoice;
}

BackwardPolicy ArraysBoundsAdvisor::dfsBackward(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsBackward(module, currentPath);
    std::cout << "ArraysBoundsAdvisor::dfsBackward" << std::endl;
    return BackwardPolicy::BP_ALLOW;
}

}
