#include "ASTManager.h"

#include <fstream>

#include "SAManager.h"
#include "macros.h"

using namespace clang;

namespace ceagle {

    void ASTManagerPrinter::findASTCallTree(ASTContext *context, const CallExpr *n) {
        //llvm::errs() << "ASTManager findASTCallTree child " << n->getDirectCallee()->getNameInfo().getName().getAsString() << "\n";
        //const Stmt *original = n;
        const Stmt *lastestStmt = n;
        std::vector<const Stmt *> stmtTrace;
        stmtTrace.push_back(n);
        while (true) {
            //get parents
            const auto &parents = context->getParents(*lastestStmt);
            if (parents.empty()) {
                llvm::errs() << "ASTManager Can not find parent\n";
                return;
            }
            //llvm::errs() << "find parent size=" << parents.size() << "\n";
            const Stmt *st = parents[0].get<Stmt>();
            if (!st) {
                const Decl *dc = parents[0].get<Decl>();
                //this->m_astManager->m_saManager->foundDecl(dc);
                this->m_astManager->m_saManager->foundStmtTrace(stmtTrace, dc);
                break;
            } else {
                //llvm::errs() << "ASTManager found " << st << "\n";
                stmtTrace.push_back(st);
                lastestStmt = st;
                //this->m_astManager->m_saManager->foundStmt(st);
            }
        }
    }

    void ASTManagerPrinter::constructCallTraces(ASTContext *context, const CallExpr *n, WitnessType type) {
        wg::Node *beforeNode;
        wg::Node *afterNode = new Node();
        if (!this->m_astManager->m_witnessNodes.empty()) {
            size_t size = this->m_astManager->m_witnessNodes.size();
            beforeNode = this->m_astManager->m_witnessNodes.at(size - 1);
        } else {
            beforeNode = new Node();
        }
        FullSourceLoc b = context->getFullLoc(n->getLocStart());
        FullSourceLoc e = context->getFullLoc(n->getLocEnd());
        if (b.isValid() && e.isValid()) {
            std::string sourceCode = std::string(b.getCharacterData(), e.getCharacterData() - b.getCharacterData());
            if (sourceCode.find(SVCOMP_INTERFACE_VERIFIER_ERROR) != std::string::npos &&
                type == WitnessType::CORRECTNESS) {
                // FIXME: a tricky method to generate correctness witness
            } else {
                wg::Edge *edge = new Edge(beforeNode, afterNode, sourceCode,
                                          context->getFullLoc(b).getSpellingLineNumber(),
                                          context->getFullLoc(b).getSpellingColumnNumber());
                if (std::find(this->m_astManager->m_witnessNodes.begin(), this->m_astManager->m_witnessNodes.end(),
                              beforeNode) == this->m_astManager->m_witnessNodes.end()) {
                    this->m_astManager->m_witnessNodes.push_back(beforeNode);
                }
                this->m_astManager->m_witnessEdges.push_back(edge);
                if (std::find(this->m_astManager->m_witnessNodes.begin(), this->m_astManager->m_witnessNodes.end(),
                              afterNode) == this->m_astManager->m_witnessNodes.end()) {
                    this->m_astManager->m_witnessNodes.push_back(afterNode);
                }
            }
        }

        const Stmt *lastestStmt = n;
        while (true) {
            // get parents
            const auto &parents = context->getParents(*lastestStmt);
            if (parents.empty()) {
                llvm::errs() << "ASTManager Can not find parent\n";
                return;
            }
            const Stmt *st = parents[0].get<Stmt>();
            if (!st) {
                break;
            } else {
                std::stack<const Stmt *> stmtStack;
                for (Stmt::const_child_iterator iter = st->child_begin(); iter != st->child_end(); ++iter) {
                    if (*iter == lastestStmt) {
                        while (!stmtStack.empty()) {
                            const Stmt *curStmt = stmtStack.top();
                            if (!curStmt) {
                                stmtStack.pop();
                                continue;
                            }
                            wg::Node *curBeforeNode;
                            wg::Node *curAfterNode = new Node();
                            if (!this->m_astManager->m_witnessNodes.empty()) {
                                size_t size = this->m_astManager->m_witnessNodes.size();
                                curBeforeNode = this->m_astManager->m_witnessNodes.at(size - 1);
                            } else {
                                curBeforeNode = new Node();
                            }
                            FullSourceLoc b = context->getFullLoc(curStmt->getLocStart());
                            FullSourceLoc e = context->getFullLoc(curStmt->getLocEnd());
                            if (b.isValid() && e.isValid()) {
                                std::string sourceCode = std::string(b.getCharacterData(),
                                                                     e.getCharacterData() - b.getCharacterData());
                                wg::Edge *edge = new Edge(curBeforeNode, curAfterNode, sourceCode,
                                                          context->getFullLoc(b).getSpellingLineNumber(),
                                                          context->getFullLoc(b).getSpellingColumnNumber());
                                if (std::find(this->m_astManager->m_witnessNodes.begin(),
                                              this->m_astManager->m_witnessNodes.end(),
                                              curBeforeNode) == this->m_astManager->m_witnessNodes.end()) {
                                    this->m_astManager->m_witnessNodes.push_back(curBeforeNode);
                                }
                                this->m_astManager->m_witnessEdges.push_back(edge);
                                if (std::find(this->m_astManager->m_witnessNodes.begin(),
                                              this->m_astManager->m_witnessNodes.end(),
                                              curAfterNode) == this->m_astManager->m_witnessNodes.end()) {
                                    this->m_astManager->m_witnessNodes.push_back(curAfterNode);
                                }
                            }
                            stmtStack.pop();
                        }
                        break;
                    } else {
                        stmtStack.push(*iter);
                    }
                }
                lastestStmt = st;
            }
        }
    }

    void ASTManagerPrinter::run(const MatchFinder::MatchResult &Result) {
        //std::cout << "Hello" << std::endl;
        if (const WhileStmt *n = Result.Nodes.getNodeAs<clang::WhileStmt>("matchWhileLoops")) {
            n->dump();
        } else if (const FunctionDecl *n = Result.Nodes.getNodeAs<clang::FunctionDecl>("matchFunction")) {
            n->dump();
        } else if (const CallExpr *n = Result.Nodes.getNodeAs<clang::CallExpr>("matchFunctionCall")) {
            //n->dump();
            findASTCallTree(Result.Context, n);
        } else if (const VarDecl *n = Result.Nodes.getNodeAs<clang::VarDecl>("matchVariableDeclaration")) {
            this->m_astManager->m_saManager->foundVariableDeclaration(n);
        } else if (const CallExpr *n = Result.Nodes.getNodeAs<clang::CallExpr>("matchViolationFunctionCallTraces")) {
            constructCallTraces(Result.Context, n, WitnessType::VIOLATION);
        } else if (const CallExpr *n = Result.Nodes.getNodeAs<clang::CallExpr>("matchCorrectnessFunctionCallTraces")) {
            constructCallTraces(Result.Context, n, WitnessType::CORRECTNESS);
        }
    }

    void ASTManager::matchWhileLoops() {
        ASTManagerPrinter printer;
        StatementMatcher matcher = whileStmt().bind("matchWhileLoops");
        MatchFinder finder;
        finder.addMatcher(matcher, &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::matchFunction(std::string name) {
        ASTManagerPrinter printer;
        DeclarationMatcher matcher = functionDecl(hasName(name)).bind("matchFunction");
        MatchFinder finder;
        finder.addMatcher(matcher, &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::matchFunctionCall(std::string name) {
        ASTManagerPrinter printer;
        printer.m_astManager = this;
        MatchFinder finder;
        finder.addMatcher(callExpr(callee(functionDecl(hasName(name)))).bind("matchFunctionCall"), &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::matchViolationFunctionCallTraces(std::string name,Witness *witness) {
        ASTManagerPrinter printer;
        printer.m_astManager = this;
        printer.m_witness = witness;
        MatchFinder finder;
        finder.addMatcher(callExpr(callee(functionDecl(hasName(name)))).bind("matchViolationFunctionCallTraces"),
                          &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::matchCorrectnessFunctionCallTraces(std::string name, Witness *witness) {
        ASTManagerPrinter printer;
        printer.m_astManager = this;
        printer.m_witness = witness;
        MatchFinder finder;
        finder.addMatcher(callExpr(callee(functionDecl(hasName(name)))).bind("matchCorrectnessFunctionCallTraces"),
                          &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::matchVariableDeclaration(std::string name) {
        ASTManagerPrinter printer;
        printer.m_astManager = this;
        MatchFinder finder;
        finder.addMatcher(varDecl(hasName(name)).bind("matchVariableDeclaration"), &printer);
        this->m_astTool->run(clang::tooling::newFrontendActionFactory(&finder).get());
    }

    void ASTManager::findFunction(std::string name) {
        std::ifstream ifs(this->m_fileName);
        std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        clang::tooling::runToolOnCode(new ASTManagerAction, file);
    }

}


