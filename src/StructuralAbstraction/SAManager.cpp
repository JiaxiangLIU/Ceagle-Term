#include "SAManager.h"

#include "debug.h"
#include "macros.h"
#include "Witness/ViolationWitness.h"
#include "Witness/CorrectnessWitness.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/FormattedStream.h"

#include <fstream>

namespace ceagle {

//void MSGManager::abstractionCheckingRefinementLoop();

void outputFunctionNames(std::vector<std::string> strings) {
    for (int i = 0; i < strings.size(); i++) {
        std::cout << strings[i] << " ";
    }
    std::cout << std::endl;
}

void SAManager::visitFunction(Function *f) {
    //llvm::outs() << "SAManager::visit function " << f->getName() << " " << f->getNumUses() << "\n";
    this->m_functionNames.push_back(f->getName());
    for (Value::user_iterator i = f->user_begin(), e = f->user_end(); i != e; ++i) {
        if (Instruction *v= dyn_cast<Instruction>(*i)) {
            Function *vf  = v->getParent()->getParent();
            //llvm::outs() << "SAManager::vf " << vf->getName() << "\n";
            this->visitFunction(vf);
        } else if (Function *v = dyn_cast<Function>(*i)) {
            llvm::outs() << "SAManager::<error>\n";
        }
    }
}

void SAManager::checkSpecial() {
    std::cout << "SAManager::checkSpecial checking " << this->m_ast->m_fileName << std::endl;
    std::ifstream ifs(this->m_ast->m_fileName);
    std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    std::cout << "SAManager::checkSpecial finished" << std::endl;
}

void SAManager::verify(llvm::Module& module) {
    llvm::outs() << "SAManager::verify file is " << this->m_ast->m_fileName << ", module is " << module.getName() << "\n";

    // some files have to be checked separatedly
    this->checkSpecial();

    // DW: TODO, for now, only start with main function
    //Function* mainFunction = module.getFunction("main");
    //MSG *root = this->m_generator->generateVCOfFunction(mainFunction);
    //this->m_manager->verify(root);

    // TODO: should output verification result here
    //this->m_ast->findFunction("main");
    //this->m_ast->matchWhileLoops();
    //this->m_ast->matchFunction("main");
    //this->m_ast->matchFunction("__VERIFIER_error");
    //this->m_ast->matchFunctionCall("__VERIFIER_error");
    //module.dump();
    llvm::Function *f = module.getFunction(SVCOMP_INTERFACE_VERIFIER_ERROR);
    this->visitFunction(f);
    llvm::outs() << "SAManager::verify functions are:\n";
    outputFunctionNames(this->m_functionNames);
    for (int i = 0; i < this->m_functionNames.size(); i++) {
        this->m_ast->matchFunctionCall(this->m_functionNames[i]);
    }
}

MSGVariableNode *makeMSGVariableNode(std::string name, const clang::Type *t) {
    MSGVariableNode *node = new MSGVariableNode(name);

    // type
    if (t->isIntegerType()) {
        node->it = MSGExpressionType::makeInt32Type();
    } else if (t->isBooleanType()) {
        node->it = MSGExpressionType::makeBoolType();
    } else if (t->isCharType()) {
        node->it = MSGExpressionType::makeCharType();
    }
    return node;
}

MSGConstantNode *makeMSGConstantNode(const clang::Expr *expr) {
    if (isa<clang::IntegerLiteral>(expr)) {
        const clang::IntegerLiteral *n = static_cast<const clang::IntegerLiteral *>(expr);
        MSGConstantNode *node = new MSGConstantNode(n->getValue().toString(10, true));
        node->it = MSGExpressionType::makeInt32Type();
        return node;
    }
}

MSGNode *constructMSG(MSG *msg, const clang::Stmt *expr, MSGConstructionContext *context) {
    if (isa<clang::ImplicitCastExpr>(expr)) {
        const clang::ImplicitCastExpr *n = static_cast<const clang::ImplicitCastExpr *>(expr);
        expr = n->getSubExprAsWritten();
    }

    if (isa<clang::BinaryOperator>(expr)) {
        const clang::BinaryOperator *n = static_cast<const clang::BinaryOperator *>(expr);
        MSGBinaryOPNode *node = new MSGBinaryOPNode();
        switch (n->getOpcode()) {
            case BO_EQ:
                node->msgOperator = MSGOperator::EQU;
                break;
            case BO_NE:
                node->msgOperator = MSGOperator::DIS_EQU;
                break;
            case BO_LT:
                node->msgOperator = MSGOperator::LESS;
                break;
            case BO_GT:
                node->msgOperator = MSGOperator::GREAT;
                break;
            case BO_LE:
                node->msgOperator = MSGOperator::LESS_EQU;
                break;
            case BO_GE:
                node->msgOperator = MSGOperator::GREAT_EQU;
                break;
            case BO_Add:
                node->msgOperator = MSGOperator::ADD;
                break;
            case BO_Sub:
                node->msgOperator = MSGOperator::MINUS;
                break;
            case BO_Mul:
                node->msgOperator = MSGOperator::MULTIPLY;
                break;
            case BO_Div:
                node->msgOperator = MSGOperator::DIVIDE;
                break;
            case BO_And:
                node->msgOperator = MSGOperator::AND;
                break;
            case BO_Xor:
                node->msgOperator = MSGOperator::XOR;
                break;
            case BO_Or:
                node->msgOperator = MSGOperator::OR;
                break;
                // DW: no NOT, IMPLICATION here
            default:
                // maybe some others
                std::cout << "SAManager::unhandled code " << n->getOpcode() << std::endl;
                break;
        }
        node->left = constructMSG(msg, n->getLHS(), context);
        node->right = constructMSG(msg, n->getRHS(), context);

        node->childtt = MSGExpressionType::getCommonType(node->left->it, node->right->it);

        if (context->m_IfStmt_Cond_Not) {
            std::cout << "SAManager::constructMSG context->m_IfStmt_Cond_Not" << std::endl;
            MSGUnaryOPNode *parentNode = new MSGUnaryOPNode();
            parentNode->msgOperator = NOT;
            parentNode->operatorNode = node;
            msg->addNode(node);
            msg->addNode(parentNode);
            std::cout << "SAManager::constructMSG returning parentNodei:" << parentNode->toString() << std::endl;
            return parentNode;
        } else {
            msg->addNode(node);
            return node;
        }
        //llvm::errs() << "SAManager::constructMSG new node " << node->toString() << "\n";
    } else if (isa<clang::IntegerLiteral>(expr)) {
        const clang::IntegerLiteral *n = static_cast<const clang::IntegerLiteral *>(expr);
        MSGConstantNode *node = new MSGConstantNode(n->getValue().toString(10, true));
        node->it = MSGExpressionType::makeInt32Type();
        msg->addNode(node);
        return node;
    } else if (isa<clang::DeclRefExpr>(expr)) {
        const clang::DeclRefExpr *n = static_cast<const clang::DeclRefExpr *>(expr);
        MSGVariableNode *node = makeMSGVariableNode(n->getNameInfo().getName().getAsString(), n->getDecl()->getType().getTypePtr());
        msg->addNode(node);
        return node;
    } else if (isa<clang::CompoundStmt>(expr)) {
        llvm::errs() << "SAManager::constructMSG no need to handle CompoundStmt\n";
    }
    return NULL;
}

void SAManager::foundDecl(const Decl *st) {
    // no use
}

void SAManager::foundIfStmt(const clang::IfStmt *ifStmt, bool isThen) {
    // no use
}

void SAManager::foundStmt(const clang::Stmt *st) {
    // no use
}

void outputStmts(std::vector<const clang::Stmt *> &st, std::string message) {
    llvm::errs() << message;
    for (int i = 0; i < st.size(); i++) {
        const clang::Stmt *s = st[i];
        llvm::errs() << i << ":\n";
        s->dump();
    }
}

void outputMSGNodes(std::vector<MSGNode *> &nodes, std::string message) {
    llvm::errs() << message;
    for (int i = 0; i < nodes.size(); i++) {
        MSGNode *n = nodes[i];
        if (n != NULL) {
            llvm::errs() << n->toString() << "\n";
        }
    }
}

// the "st" stores Stmt in reverse order, [0]:CallExpr, [1]:Parent, ...
void SAManager::translate(std::vector<MSGNode *> &nodes, std::vector<const clang::Stmt *> &st) {
    //outputStmts(st, "SAManager::translate stmts\n");
    for (int i = 0; i < st.size(); i++) {
        const clang::Stmt *s = st[i];
        if (isa<clang::IfStmt>(s)) {
            //llvm::errs() << "SAManager::translate found a IfStmt\n";
            const clang::IfStmt *ifStmt = static_cast<const clang::IfStmt *>(s);

            const Stmt *thenStmt = ifStmt->getThen();
            const Stmt *elseStmt = ifStmt->getElse();
            const Stmt *next = st[i-1];
            MSGConstructionContext *context = new MSGConstructionContext();
            if (thenStmt == next) {
                context->m_IfStmt_Cond_Not = false;
            } else if (elseStmt == next) {
                context->m_IfStmt_Cond_Not = true;
            } else {
                std::cout << "SAManager::not a then or else" << std::endl;
            }
            MSGNode *n = constructMSG(this->m_manager->msg, ifStmt->getCond(), context);
            if (n != NULL) {
                //llvm::errs() << "SAManager::translate constructed node " << n->toString() << "\n";
            }
            nodes.push_back(n);
        }
    }
    //outputMSGNodes(nodes, "SAManager::translate nodes\n");
}

//void SAManager::foundStmtTrace(std::vector<const clang::Stmt *> &st, const clang::Decl * dc) {
//}

void SAManager::combineConstructionFunctions() {
    for (int i = 0; i < this->m_msgConstructionFunctions.size(); i++) {
        //llvm::errs() << "SAManager::foundStmtTrace " << this->m_msgConstructionFunctions[i] << "\n";
        MSGConstructionFunction *msgCF = this->m_msgConstructionFunctions[i];
        //llvm::errs() << "SAManager::combineConstructionFunctions size is " << msgCF->m_msgNodes.size() << "\n";
        if ((msgCF == NULL) || (msgCF->m_msgNodes.size() <= 0)) {
            continue;
        }
        for (int j = 0; j < msgCF->m_msgNodes.size() - 1; j++) {
            MSGNode *n1 = msgCF->m_msgNodes[j];
            MSGNode *n2 = msgCF->m_msgNodes[j+1];
            MSGBinaryOPNode *node = new MSGBinaryOPNode();
            node->msgOperator = MSGOperator::AND;
            node->left = n1;
            node->right = n2;
            llvm::errs() << "SAManager::combineConstructionFunctions new node " << node->toString() << "\n";
            this->m_manager->msg->addNode(node);
        }
    }
}

void SAManager::foundStmtTrace(std::vector<const clang::Stmt *> &st, const clang::Decl * dc) {
    if (isa<clang::FunctionDecl>(dc)) {
        const clang::CallExpr *child = static_cast<const clang::CallExpr *>(st[0]);
        const clang::FunctionDecl *parent = static_cast<const clang::FunctionDecl *>(dc);
        std::string fName = parent->getNameInfo().getName().getAsString();
        llvm::errs() << "SAManager::foundStmtTrace "<< child->getDirectCallee()->getNameInfo().getName().getAsString() << " called by " << fName << "\n";

#if DEBUG_AST
        llvm::errs() << "---AST " << st.size() << " begin---\n";
        outputStmts(st, "");
        llvm::errs() << "---AST end---\n";
#endif

        MSGConstructionFunction *cf = new MSGConstructionFunction();
        cf->m_functionChild = child;
        cf->m_functionParent = parent;
        this->translate(cf->m_msgNodes, st);
#if DEBUG_MSG
        //llvm::errs() << "---MSG " << cf->m_msgNodes.size() << " begin---\n";
        //outputMSGNodes(cf->m_msgNodes, "");
        //llvm::errs() << "---MSG end---\n";
#endif
        this->m_msgConstructionFunctions.push_back(cf);
        if (fName == "main") {
            //llvm::errs() << "SAManager::foundStmtTrace found main function with depth " << this->m_msgConstructionFunctions.size() << "\n";
            this->combineConstructionFunctions();

            // find variables
            for (int i = 0; i < this->m_manager->msg->leafNodes.size(); i++) {
                if (MSGVariableNode *vn = dynamic_cast<MSGVariableNode *>(this->m_manager->msg->leafNodes[i])) {
                    //llvm::outs() << "SAManager::foundStmtTrace found MSGVariableNode " << vn->toString() << "\n";
                    this->m_ast->matchVariableDeclaration(vn->name);
                    break;
                }
            }

            return;
        }
    }
}

void SAManager::generateViolationWitness() {
    Witness *violationWitness = new ViolationWitness(this->m_ast->m_fileName);

    if (m_functionNames.size() <= 0) {
        llvm::errs() << "SAManager::generateViolationWitness no error path!";
        return;
    }

    for (std::vector<string>::const_iterator iter = m_functionNames.cbegin();
         iter != m_functionNames.cend(); iter++) {
        std::string lastCallFunName = *iter;
        this->m_ast->matchViolationFunctionCallTraces(lastCallFunName, violationWitness);
    }

    std::vector<Node *> beforeNodes;
    for (int i = this->m_ast->m_witnessEdges.size() - 1; i >= 0; i--) {
        wg::Node *beforeNode;
        if (beforeNodes.size() == 0) {
            beforeNode = new Node();
            beforeNode->isEntry = true;
            violationWitness->add(beforeNode);
        } else {
            beforeNode = beforeNodes.at(beforeNodes.size() - 1);
        }
        wg::Node *afterNode = new Node();
        if (i == 0) {
            afterNode->isViolation = true;
        }
        beforeNodes.push_back(afterNode);
        violationWitness->add(afterNode);
        wg::Edge *tmpEdge = this->m_ast->m_witnessEdges.at(i);
        wg::Edge *edge = new Edge(beforeNode, afterNode, tmpEdge->srcCode, tmpEdge->line, tmpEdge->column);
        violationWitness->add(edge);
    }

    std::string xmlContent = violationWitness->getWitness();
//    std::string xmlFileName = this->m_ast->m_fileName + "_FALSE.graphml"; // debug mode format
    std::string xmlFileName = "witness.graphml"; // competition mode format
    std::ofstream out(xmlFileName);
    if (out.is_open()) {
        out << xmlContent;
        out.close();
    }
}

void SAManager::generateCorrectnessWitness() {
    Witness *correctnessWitness = new CorrectnessWitness(this->m_ast->m_fileName);

    if (m_functionNames.size() <= 0) {
        llvm::errs() << "SAManager::generateCorrectnessWitness no possible executable path!";
        return;
    }

    for (std::vector<string>::const_iterator iter = m_functionNames.cbegin();
         iter != m_functionNames.cend(); iter++) {
        std::string lastCallFunName = *iter;
        this->m_ast->matchCorrectnessFunctionCallTraces(lastCallFunName, correctnessWitness);
    }

    std::vector<Node *> beforeNodes;
    for (int i = this->m_ast->m_witnessEdges.size() - 1; i >= 0; i--) {
        wg::Node *beforeNode;
        if (beforeNodes.size() == 0) {
            beforeNode = new Node();
            beforeNode->isEntry = true;
            correctnessWitness->add(beforeNode);
        } else {
            beforeNode = beforeNodes.at(beforeNodes.size() - 1);
        }
        wg::Node *afterNode = new Node();
        beforeNodes.push_back(afterNode);
        correctnessWitness->add(afterNode);
        wg::Edge *tmpEdge = this->m_ast->m_witnessEdges.at(i);
        wg::Edge *edge = new Edge(beforeNode, afterNode, tmpEdge->srcCode, tmpEdge->line, tmpEdge->column);
        correctnessWitness->add(edge);
    }

    std::string xmlContent = correctnessWitness->getWitness();
//    std::string xmlFileName = this->m_ast->m_fileName + "_TRUE.graphml"; // debugo mode format
    std::string xmlFileName = "witness.graphml"; // competition mode format
    std::ofstream out(xmlFileName);
    if (out.is_open()) {
        out << xmlContent;
        out.close();
    }
}

void SAManager::foundVariableDeclaration(const clang::VarDecl * dc) {
    if (dc->hasInit()) {
        const clang::Expr *e = dc->getInit();
        //llvm::outs() << "SAManager::foundVariableDeclaration the init:\n";
        //e->dump();

        // construct the binary node
        MSGBinaryOPNode *node = new MSGBinaryOPNode();
        node->msgOperator = MSGOperator::EQU;
        node->left = makeMSGVariableNode(dc->getNameAsString(), dc->getType().getTypePtr());
        node->right = makeMSGConstantNode(dc->getInit());

        MSGBinaryOPNode *vc = new MSGBinaryOPNode();
        vc->msgOperator = MSGOperator::AND;
        vc->left = node;
        outputMSGNodes(this->m_manager->msg->nonLeafNodes, "this->m_manager->msg->nonLeafNodes:");
        vc->right = this->m_manager->msg->nonLeafNodes.back();

        this->m_manager->msg->addNode(node->left);
        this->m_manager->msg->addNode(node->right);
        this->m_manager->msg->addNode(node);
        this->m_manager->msg->addNode(vc, true);

        llvm::errs() << "SAManager::foundVariableDeclaration constucted:\n" << this->m_manager->msg->toString();
        this->m_manager->abstractionCheckingRefinementLoop();
    }
}

}
