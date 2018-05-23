#ifndef AST_MANAGER_H
#define AST_MANAGER_H

#include <iostream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"

#include "Witness/Witness.h"

//#include "SAManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace ceagle {

template<typename NodeT>
void getParentOf(ASTContext *context, NodeT &n) {
//    while (true) {
//        //get parents
//        const auto& parents = context->getParents(n);
//        if (parents.empty()) {
//            llvm::errs() << "Can not find parent\n";
//            return;
//        }
//        llvm::errs() << "find parent size=" << parents.size() << "\n";
//        const Stmt* st = parents[0].get<Stmt>();
//        if (!st) {
//            return;
//        }
//        st->dump();
//        if (isa<CompoundStmt>(st)) {
//            break;
//        }
//    } 
}

class ASTManagerVisitor : public RecursiveASTVisitor<ASTManagerVisitor> {
public:
    explicit ASTManagerVisitor(ASTContext *Context) : Context(Context) {}

    bool VisitFunctionDecl(FunctionDecl *Declaration) {
        //std::cout << "ASTManagerVisitor hahahaha" << std::endl;
        if (Declaration->getQualifiedNameAsString() == "main") {
            FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getLocStart());
            if (FullLocation.isValid()) {
                llvm::outs() << "Found declaration at " << FullLocation.getSpellingLineNumber() << ":" << FullLocation.getSpellingColumnNumber() << "\n";
            }
            //getParentOf(Context, Declaration);

        }
        return true;
    }

private:
    ASTContext *Context;
};

class ASTManagerConsumer : public clang::ASTConsumer {
public:
    explicit ASTManagerConsumer(ASTContext *Context) : Visitor(Context) {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        //std::cout << "ASTManagerConsumer hahahaha" << std::endl;
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
private:
    ASTManagerVisitor Visitor;
};

// this class is not used, indeed
class ASTManagerAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
        //std::cout << "ASTManagerAction hahahaha" << std::endl;
        Compiler.getDiagnostics().setClient(new clang::IgnoringDiagConsumer(), true);
        return std::unique_ptr<clang::ASTConsumer>(new ASTManagerConsumer(&Compiler.getASTContext()));
    }
};

class ASTManager;
class ASTManagerPrinter : public MatchFinder::MatchCallback {
public :
    ASTManager *m_astManager;
    Witness *m_witness;
    void run(const MatchFinder::MatchResult &Result);
    void findASTCallTree(ASTContext *context, const CallExpr *n);
    void constructCallTraces(ASTContext *context, const CallExpr *n,WitnessType type);
};

class SAManager;
class ASTManager {
public:
    std::string m_fileName;
    SAManager *m_saManager;
    std::vector<wg::Node *> m_witnessNodes;
    std::vector<wg::Edge *> m_witnessEdges;

    clang::tooling::ClangTool *m_astTool;
    clang::tooling::CommonOptionsParser *m_astOptionsParser;
	ASTManager(std::string fileName, SAManager *saManager) : m_fileName(fileName), m_saManager(saManager) {
        // make a customized clang tool

        int argSize = 3;
        int argc = argSize;
        const char **argv = (const char **)malloc(argSize);
        argv[0] = "clang";
        argv[1] = this->m_fileName.c_str();
        argv[2] = "--";

        llvm::cl::OptionCategory astCategory("ceagle");
        this->m_astOptionsParser = new clang::tooling::CommonOptionsParser(argc, argv, astCategory);
        this->m_astTool = new clang::tooling::ClangTool(this->m_astOptionsParser->getCompilations(), this->m_astOptionsParser->getSourcePathList());
        // no warnings
        this->m_astTool->setDiagnosticConsumer(new clang::IgnoringDiagConsumer());
	}
    void findFunction(std::string name);
    void matchWhileLoops();
    void matchFunction(std::string name);
    void matchFunctionCall(std::string name);
    void matchViolationFunctionCallTraces(std::string name, Witness *witness);
    void matchCorrectnessFunctionCallTraces(std::string name, Witness *witness);
    void matchVariableDeclaration(std::string name);
};
	
}

#endif
