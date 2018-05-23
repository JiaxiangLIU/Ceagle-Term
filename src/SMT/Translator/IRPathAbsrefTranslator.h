#ifndef CEAGLE_IR_PATH_ABSREF_TRANSLATOR_H
#define CEAGLE_IR_PATH_ABSREF_TRANSLATOR_H

#include "Translator.h"

using llvm::Value;
using std::vector;
using std::map;
using ceagle::smt::Expr;
using ceagle::smt::Var;

namespace ceagle {

    class IRPathAbsrefTranslator : public Translator<> {
        Expr getExpr(const Value *val);

    public:
        virtual Expr path2SMTM(const Module &g, const vector<const BasicBlock *> path, int firstPHISelector = -1);
        virtual Expr path2SMTM(const Module &g, const vector<const BasicBlock *> path, map<const Value *, Expr *> &valueExprMap,
                  int firstPHISelector = -1) {}
    };

}

#endif
