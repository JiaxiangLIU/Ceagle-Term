#ifndef CEAGLE_ANALYSIS_H
#define CEAGLE_ANALYSIS_H

#include <llvm/IR/Module.h>
#include "DFS/DFS.h"

namespace ceagle {

class Analysis {
public:
    virtual void start(llvm::Module& module) = 0;
    virtual DFSResult getResult() = 0;
}; // class Analysis

} // namespace ceagle
#endif
