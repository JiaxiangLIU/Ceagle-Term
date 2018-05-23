/*
 * SEGState.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jiaxiang
 */

#ifndef SRC_TERMINATION_SEGSTATE_H_
#define SRC_TERMINATION_SEGSTATE_H_

#include <llvm/IR/BasicBlock.h>
#include "SMT/Middleware/AST.h"
#include "BasicTypes.h"
#include "StackFrame.h"
#include "SMT/Middleware/Solver.h"


namespace ceagle {

class SEGState {
public:
    const ProgramCounter pc;
    LocalVars LVi;
    AllocationList ALi;

    ExprSet KB;
    AllocationList AL;
    PointerTable PT;
    std::vector<StackFrame> CS;

    ExprSet FO;

    SEGState(ProgramCounter p);

    void show();
    ExprSet getPredicate(smt::Solver* solver);
    ExprSet extendPredicate(smt::Solver* solver);
    smt::Expr getLVi(llvm::Value* v);
    Domain getDomain();

    Substitution computeMu(LocalVars lv); // compute mu : this.lv -> lv
    bool isGeneralizationOf(SEGState* s, smt::Solver* solver);

private:
    /*
    bool isEq(const smt::Expr& e);
    bool isNEq(const smt::Expr& e);
    std::pair<smt::Expr, smt::Expr> splitEq(const smt::Expr& eq);
    std::pair<smt::Expr, smt::Expr> splitNEq(const smt::Expr& neq);
    */

};

}

#endif /* SRC_TERMINATION_SEGSTATE_H_ */
