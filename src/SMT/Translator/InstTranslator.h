#ifndef CEAGLE_INST_TRANSLATOR_H
#define CEAGLE_INST_TRANSLATOR_H

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Type.h>

#include "SMT/Middleware/AST.h"

namespace ceagle {

using namespace smt;

#if 0
class InstTranslator {
    size_t nonDetCount = 0;
    std::map<std::string, Expr> valueMap;
    Expr constantToValue(llvm::Constant* constant);
public:
    Expr translate(llvm::Value* value);
    Expr translate(llvm::Instruction* inst);
    std::map<std::string, Expr> getValueMap() const {
        return valueMap;
    }
};
#endif

class TranslatorVisitor : public llvm::InstVisitor<TranslatorVisitor> {
    std::map<std::string, llvm::Value*> nameMap;
    std::map<llvm::Value*, Expr> valueMap;
    Expr br = BoolValue(true);
    Expr rangeConstraint = BoolValue(true);
    int phiSelector = -1;
    bool encodeIntAsBv = true;
    size_t nonDetCount = 0;

    void createVar(llvm::Instruction* I, llvm::Type* type);
    void createVar(llvm::Instruction* I);
    void createPointerVar(llvm::Instruction* I);
    Sort typeToSort(llvm::Type* type);
    Expr constantToExpr(llvm::Constant* constant);
public:
    void setPhiSelector(int i) { phiSelector = i; }
    Expr get(std::string name);
    Expr get(llvm::Value*);
    Expr getCondition() { return br; }
    Expr getRangeConstraint() { return rangeConstraint; }
    std::map<llvm::Value*, Expr> getValueMap() {
        return valueMap;
    }
    void set(llvm::Value*, const Expr&);
    void visitAllocaInst (llvm::AllocaInst &I);
    void visitStoreInst (llvm::StoreInst &I);
    void visitLoadInst (llvm::LoadInst &I);
    void visitICmpInst (llvm::ICmpInst &I);
    void visitSExtInst (llvm::SExtInst &I);
    void visitZExtInst (llvm::ZExtInst &I);
    void visitTruncInst (llvm::TruncInst &I);
    void visitCallInst (llvm::CallInst &I);
    void visitBinaryOperator (llvm::BinaryOperator &I);
    void visitBranchInst (llvm::BranchInst &I);
    void visitPHINode (llvm::PHINode &I);
};

class VisitorTranslator {
    TranslatorVisitor visitor;
public:
    virtual Expr path2SMTM(std::vector<llvm::BasicBlock*> path, int firstPHISelector = -1);
    virtual Expr path2SMTM(std::vector<llvm::BasicBlock*> path, std::map<llvm::Value*, Expr>& valueExprMap, int firstPHISelector = -1);
};

}
#endif
