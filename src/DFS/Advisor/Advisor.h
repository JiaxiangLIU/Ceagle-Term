#ifndef CEAGLE_ADVISOR_H
#define CEAGLE_ADVISOR_H

#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h>

// DW: test only
#include "debug.h"
#include <iostream>

#define STR_LABEL_ERROR "PREPROCESS_LABEL_ERROR"
#define STR_LABEL_ASSUME_FAIL "PREPROCESS_LABEL_ASSUME_FAIL"

using llvm::Module;
using llvm::BasicBlock;

namespace ceagle {
enum class BackwardPolicy {
    BP_NULL,
    BP_ALLOW,
    BP_FORBIDDEN
};

// flags of Advisor::dfsNext
enum class NextPolicy {
    NP_NULL,
    NP_CONTINUE_NO_ADVICE,
    NP_CONTINUE_DETERMINED_ADVICE,
    NP_CONTINUE_NONDETERMINED_ADVICE,
    NP_BREAK_UNSAFE,
    NP_BREAK_SAFE,
    NP_BREAK_UNKNOWN
};

template<class T>
struct NextChoice {
    NextPolicy flag;
    std::vector<T> choices;
};

typedef const BasicBlock* P_BB;
typedef const std::vector<P_BB>& R_BBPATH;

template<class T = P_BB>
class Advisor {
public:
    virtual void dfsVisit(const Module& module, const std::vector<T>& currentPath) {
        // DW: test
        //std::cout << "Advisor::dfsVisit" << std::endl;

        return;
    }
    virtual NextChoice<T> dfsNext(const Module& module, const std::vector<T>& currentPath) {
        // DW: test
        //std::cout << "Advisor::dfsNext" << std::endl;

        return {NextPolicy::NP_CONTINUE_NO_ADVICE, std::vector<T>()};
    }

    virtual BackwardPolicy dfsBackward(const Module& module, const std::vector<T>& currentPath) {
        // DW: test
        //std::cout << "Advisor::dfsBackward" << std::endl;

        return BackwardPolicy::BP_ALLOW;
    }
    virtual ~Advisor() = default;
};

Advisor<>* makeAdvisor(const std::string& name);
}
#endif
