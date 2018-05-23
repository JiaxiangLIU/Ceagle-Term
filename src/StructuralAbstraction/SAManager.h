#ifndef SA_MANAGER_H
#define SA_MANAGER_H

#include "llvm/IR/Module.h"

#include "VCGenerator.h"
#include "ASTManager.h"
#include "MSGManager.h"

using namespace llvm;

namespace ceagle {

class MSGConstructionContext {
public:
    const clang::Stmt *m_st;
    bool m_IfStmt_Cond_Not;
    MSGConstructionContext() {
        m_st = NULL;
        m_IfStmt_Cond_Not = false;
    }
};

class MSGConstructionFunction {
public:
    const clang::CallExpr *m_functionChild;
    const clang::FunctionDecl *m_functionParent;
    // the call condition
    std::vector<MSGNode *> m_msgNodes;
    //MSGNode * m_msgNodesHead;
};

class SAManager {
    VCGenerator *m_generator;
    ASTManager *m_ast;
    MSGManager *m_manager;

    // handling function call tree
    std::vector<std::string> m_functionNames;
    void visitFunction(Function *f);

    // new design
    //std::vector<MSGNode *> m_
    std::vector<MSGConstructionFunction *> m_msgConstructionFunctions;
public:
	SAManager() {
        this->m_generator = new VCGenerator();
        this->m_manager = new MSGManager(this);
	}
	SAManager(std::string fileName) {
        this->m_generator = new VCGenerator();
        this->m_ast = new ASTManager(fileName, this);
        this->m_manager = new MSGManager(this);
	}
	void verify(llvm::Module& module);

    void checkSpecial();

    // AST functions
    void foundDecl(const clang::Decl *st);
    void foundIfStmt(const clang::IfStmt *ifStmt, bool isThen);
    void foundStmt(const clang::Stmt *st);

    // new design
    void foundStmtTrace(std::vector<const clang::Stmt *> &st, const clang::Decl * dc);
    void foundVariableDeclaration(const clang::VarDecl * dc);
    void translate(std::vector<MSGNode *> &nodes, std::vector<const clang::Stmt *> &st);

    // generate fail witness
    void generateViolationWitness();

    // generate correct witness
    void generateCorrectnessWitness();

    void combineConstructionFunctions();
};

}

#endif
