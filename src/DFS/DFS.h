#ifndef CEAGLE_DFS_H
#define CEAGLE_DFS_H

#include <llvm/IR/BasicBlock.h>

#include "Advisor/Advisor.h"

using llvm::Module;
using llvm::BasicBlock;

namespace ceagle {

enum class DFSResult {
    DR_NULL,
    DR_UNSAFE,
    DR_SAFE,
	DR_UNKNOWN
};

template<class NodeTy=BasicBlock, class GraphTy=Module>
class DFS {
protected:
    Advisor<const NodeTy*> *advisor;
public:
    DFS(Advisor<const NodeTy*> *advisor) {
        this->advisor = advisor;
    }
    virtual ~DFS() = default;
    virtual void visit(const GraphTy& g) {
    }
	virtual DFSResult getResult() {
	}
};

}
#endif
