#ifndef CEAGLE_TRANSLATOR_H
#define CEAGLE_TRANSLATOR_H

#include "../Middleware/AST.h"

#include <llvm/IR/BasicBlock.h>

#include <map>

using llvm::Module;
using llvm::BasicBlock;
using llvm::Value;
using ceagle::smt::Expr;
using std::vector;
using std::map;

namespace ceagle {

template<class NodeTy=BasicBlock, class GraphTy=Module>
class Translator {
public:
    virtual Expr path2SMTM(const GraphTy& g, const vector<const NodeTy*> path, int firstPHISelector = -1) {
    }
	virtual Expr path2SMTM(const GraphTy& g, const vector<const NodeTy*> path, map<const Value*, Expr*>& valueExprMap, int firstPHISelector = -1) {
    }
    virtual ~Translator() = default;
};

}
#endif
