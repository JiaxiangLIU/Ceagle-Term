#include "MSGManager.h"

#include "VCGenerator.h"
#include "SMT/Middleware/AST.h"
#include "SAManager.h"
#include "Witness/ViolationWitness.h"

#define DEBUG_MSGMANAGER 1

using namespace ceagle::smt;
using namespace ceagle;
using std::string;
using std::stringstream;

namespace ceagle {

    std::string concatenate(std::string const &str, int integer) {
        std::stringstream ss;
        ss << str << integer;
        return ss.str();
    }

    smt::Expr *MSGManager::MSGLeafNode2smt(MSGNode *msgNode) {
        smt::Expr *expr;
        MSGVarType varType = msgNode->it.type;
        if (msgNode->isLeafNode()) {
            // leaf node may be variable or constant,summary
            if (MSGSummaryNode *summaryNode = dynamic_cast<MSGSummaryNode *>(msgNode)) {
                switch (varType) {
                    case INT:
                        expr = new Var(concatenate(MSGNODE_SMT_PREFIX, summaryNode->index), IntSort());
                        break;
                    case BOOL:
                        expr = new Var(concatenate(MSGNODE_SMT_PREFIX, summaryNode->index), BoolSort());
                        break;
                    case FLOAT:
                        expr = new Var(concatenate(MSGNODE_SMT_PREFIX, summaryNode->index), FloatSort());
                        break;
                        // TODO support array,bitvector,real
                }
            } else if (MSGVariableNode *variableNode = dynamic_cast<MSGVariableNode *> (msgNode)) {
                // DW: let's try not using "smt_x" for leafNodes
                switch (varType) {
                    case INT:
                        expr = new Var(variableNode->name, IntSort());
                        break;
                    case BOOL:
                        expr = new Var(variableNode->name, BoolSort());
                        break;
                    case FLOAT:
                        expr = new Var(variableNode->name, FloatSort());
                        break;
                        // TODO support array,bitvector,real
                }
            } else if (MSGConstantNode *constantNode = dynamic_cast<MSGConstantNode *>(msgNode)) {
                switch (varType) {
                    case INT:
                        expr = new IntValue(std::stoi(constantNode->value));
                        break;
                    case BOOL:
                        if (constantNode->value == "true") {
                            expr = new BoolValue(true);
                        } else if (constantNode->value == "false") {
                            expr = new BoolValue(false);
                        } else {
                            throw "Illegal MSGConstantNode bool type:" + constantNode->value;
                        }
                        break;
                    case FLOAT:
                        expr = new FloatValue(std::stof(constantNode->value));
                        break;
                        // TODO support array,bitvector,real
                }
            } else {
                throwStrForValue("[MSGManager.cpp MSGNode2smt] Cannot get expr of msgNode", msgNode);
            }
        } else {
            throwStrForValue("[MSGManager.cpp MSGLeafNode2smt] Doesn't support nonleaf node!", msgNode);
        }


        return expr;
    }

    /**
     * FIXME Node can be transform to IntSort or BvSort value accroding to msg size!
     * @see 20160924 structural-abstraction-memo.txt
     *
     * @param msgNode
     * @param exprs
     * @return
     */
    void MSGManager::MSGNode2smt(MSGNode *msgNode, std::vector<smt::Expr *> &exprs) {
        if (msgNode->hasTransformed) {
            return;
        } else {
            msgNode->hasTransformed = true;
        }

        smt::Expr *expr;
        if (msgNode->isLeafNode()) {
            return;
        } else {
            // recursively solve the MSGNode
            smt::Expr *curNodeExpr;
            MSGVarType curNodeType = msgNode->it.type;
            switch (curNodeType) {
                case BOOL:
                    curNodeExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, msgNode->index), BoolSort());
                    break;
                case INT:
                    curNodeExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, msgNode->index), IntSort());
                    break;
                case FLOAT:
                    curNodeExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, msgNode->index), FloatSort());
                    break;
                default:
                    // TODO support array,bitvector,real
                    break;
            }

            if (MSGVCNode *vcNode = dynamic_cast<MSGVCNode *>(msgNode)) {
                MSGNode *leftNode = vcNode->leftNode;
                smt::Expr *leftExpr;
                if (leftNode->isLeafNode()) {
                    leftExpr = MSGLeafNode2smt(leftNode);
                } else {
                    leftExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, leftNode->index), BoolSort());
                }
                MSGNode *rightNode = vcNode->rightNode;
                smt::Expr *rightExpr;
                if (rightNode->isLeafNode()) {
                    rightExpr = MSGLeafNode2smt(rightNode);
                } else {
                    rightExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, rightNode->index), BoolSort());
                }

                MSGNode2smt(leftNode, exprs);
                MSGNode2smt(rightNode, exprs);
                expr = new smt::Expr((*curNodeExpr) == (!(*leftExpr) || (*rightExpr)));
            } else if (MSGUnaryOPNode *unaryNode = dynamic_cast<MSGUnaryOPNode *>(msgNode)) {
                MSGOperator op = unaryNode->getMSGOperator();
                MSGNode *operandNode = unaryNode->operatorNode;
                MSGVarType varType = unaryNode->childtt.type;
                switch (op) {
                    case ADD:
                        if (varType == BOOL) {
                            throwStrForValue(
                                    "[MSGManager.cpp MSGNode2smt] ADD operator can't apply for BOOL type!",
                                    msgNode);
                        } else if (varType == INT) {
                            if (operandNode->isLeafNode()) {
                                expr = new smt::Expr((*curNodeExpr) == (*MSGLeafNode2smt(operandNode)));
                            } else {
                                expr = new smt::Expr((*curNodeExpr) ==
                                                     (Var(concatenate(MSGNODE_SMT_PREFIX, operandNode->index),
                                                          IntSort())));
                            }
                        } else if (varType == FLOAT) {
                            if (operandNode->isLeafNode()) {
                                expr = new smt::Expr((*curNodeExpr) == (*MSGLeafNode2smt(operandNode)));
                            } else {
                                expr = new smt::Expr((*curNodeExpr) ==
                                                     (Var(concatenate(MSGNODE_SMT_PREFIX, operandNode->index),
                                                          FloatSort())));
                            }
                        } else {
                            // TODO support array,bitvector,real
                        }
                        break;
                    case MINUS:
                        if (varType == BOOL) {
                            throwStrForValue(
                                    "[MSGManager.cpp MSGNode2smt] MINUS operator can't apply for BOOL type!",
                                    msgNode);
                        } else if (varType == INT) {
                            smt::Expr intZeroExpr = IntValue(0);
                            if (operandNode->isLeafNode()) {
                                expr = new smt::Expr((*curNodeExpr) == (intZeroExpr - (*MSGLeafNode2smt(operandNode))));
                            } else {
                                expr = new smt::Expr((*curNodeExpr) ==
                                                     (intZeroExpr -
                                                      Var(concatenate(MSGNODE_SMT_PREFIX, operandNode->index),
                                                          IntSort())));
                            }
                        } else if (varType == FLOAT) {
                            smt::Expr floatZeroExpr = FloatValue(0.0f);
                            if (operandNode->isLeafNode()) {
                                expr = new smt::Expr(
                                        (*curNodeExpr) == (floatZeroExpr - (*MSGLeafNode2smt(operandNode))));
                            } else {
                                expr = new smt::Expr(
                                        (*curNodeExpr) == (floatZeroExpr - Var(concatenate(MSGNODE_SMT_PREFIX,
                                                                                           operandNode->index),
                                                                               FloatSort())));
                            }
                        } else {
                            // TODO support array,bitvector,real
                        }
                        break;
                    case NOT:
                        if (varType == BOOL) {
                            if (operandNode->isLeafNode()) {
                                expr = new smt::Expr((*curNodeExpr) == (!(*MSGLeafNode2smt(operandNode))));
                            } else {
                                expr = new smt::Expr(
                                        (*curNodeExpr) == (!Var(concatenate(MSGNODE_SMT_PREFIX, operandNode->index),
                                                                BoolSort())));
                            }
                        } else {
                            throwStrForValue(
                                    "[MSGManager.cpp MSGNode2smt] NOT operator can only apply for BOOL type!",
                                    msgNode);
                        }
                        break;
                }
                MSGNode2smt(operandNode, exprs);
            } else if (MSGBinaryOPNode *binaryNode = dynamic_cast<MSGBinaryOPNode *>(msgNode)) {
                MSGNode *leftNode = binaryNode->left;
                MSGNode *rightNode = binaryNode->right;
                MSGOperator op = binaryNode->getMSGOperator();
                MSGVarType varType = binaryNode->childtt.type;
                smt::Expr *leftExpr;
                if (leftNode->isLeafNode()) {
                    leftExpr = MSGLeafNode2smt(leftNode);
                } else {
                    switch (varType) {
                        case BOOL:
                            leftExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, leftNode->index), BoolSort());
                            break;
                        case INT:
                            leftExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, leftNode->index), IntSort());
                            break;
                        case FLOAT:
                            leftExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, leftNode->index), FloatSort());
                            break;
                        default:
                            // TODO support array,bitvector,real
                            break;
                    }
                }
                smt::Expr *rightExpr;
                if (rightNode->isLeafNode()) {
                    rightExpr = MSGLeafNode2smt(rightNode);
                } else {
                    switch (varType) {
                        case BOOL:
                            rightExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, rightNode->index), BoolSort());
                            break;
                        case INT:
                            rightExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, rightNode->index), IntSort());
                            break;
                        case FLOAT:
                            rightExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, rightNode->index), FloatSort());
                        default:
                            // TODO support array,bitvector,real
                            break;
                    }
                }

                switch (op) {
                    case EQU:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) == (*rightExpr)));
                        break;
                    case DIS_EQU:
                        expr = new smt::Expr((*curNodeExpr) == (!((*leftExpr) == (*rightExpr))));
                        break;
                    case LESS:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) < (*rightExpr)));
                        break;
                    case GREAT:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) > (*rightExpr)));
                        break;
                    case LESS_EQU:
                        expr = new smt::Expr((*curNodeExpr) == (bvule(*leftExpr, *rightExpr)));
                        break;
                    case GREAT_EQU:
                        expr = new smt::Expr((*curNodeExpr) == (bvuge(*leftExpr, *rightExpr)));
                        break;
                    case ADD:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) + (*rightExpr)));
                        break;
                    case MINUS:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) - (*rightExpr)));
                        break;
                    case MULTIPLY:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) * (*rightExpr)));
                        break;
                    case DIVIDE:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) / (*rightExpr)));
                        break;
                    case OR:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) || (*rightExpr)));
                        break;
                    case AND:
                        expr = new smt::Expr((*curNodeExpr) == ((*leftExpr) && (*rightExpr)));
                        break;
                }
                MSGNode2smt(leftNode, exprs);
                MSGNode2smt(rightNode, exprs);
            } else if (MSGITEOPNode *iteNode = dynamic_cast<MSGITEOPNode *>(msgNode)) {
                MSGNode *condNode = iteNode->condNode;
                MSGNode *trueNode = iteNode->trueNode;
                MSGNode *falseNode = iteNode->falseNode;
                MSGVarType varType = iteNode->childtt.type;
                smt::Expr *condExpr;
                if (condNode->isLeafNode()) {
                    condExpr = MSGLeafNode2smt(condNode);
                } else {
                    condExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, condNode->index), BoolSort());
                }
                smt::Expr *trueExpr;
                if (trueNode->isLeafNode()) {
                    trueExpr = MSGLeafNode2smt(trueNode);
                } else {
                    switch (varType) {
                        case BOOL:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, trueNode->index), BoolSort());
                            break;
                        case INT:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, trueNode->index), IntSort());
                            break;
                        case FLOAT:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, trueNode->index), FloatSort());
                            break;
                        default:
                            // TODO support array,bitvector,real
                            break;
                    }
                }
                smt::Expr *falseExpr;
                if (falseNode->isLeafNode()) {
                    falseExpr = MSGLeafNode2smt(falseNode);
                } else {
                    switch (varType) {
                        case BOOL:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, falseNode->index), BoolSort());
                            break;
                        case INT:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, falseNode->index), IntSort());
                            break;
                        case FLOAT:
                            trueExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, falseNode->index), FloatSort());
                            break;
                        default:
                            // TODO support array,bitvector,real
                            break;
                    }
                }
                expr = new smt::Expr((*curNodeExpr) == (ite(*condExpr, *trueExpr, *falseExpr)));
                MSGNode2smt(condNode, exprs);
                MSGNode2smt(trueNode, exprs);
                MSGNode2smt(falseNode, exprs);
            } else {
                throwStrForValue("[MSGManager.cpp MSGNode2smt] Cannot support node type of msgNode", msgNode);
            }
            exprs.push_back(expr);
        }
    }

    /**
     * calculate the smt formula of vcNodes
     *
     * @param msg
     * @return
     */
    std::vector<MSGNodePair> MSGManager::MSG2smt() {
        std::vector<MSGNodePair> result;
        std::vector<MSGNode *> vcNodes = (*msg).getVcNodes();

        for (std::vector<MSGNode *>::const_iterator iter = vcNodes.cbegin(); iter != vcNodes.cend(); iter++) {
            MSGNode *node = (*iter);
            std::vector<smt::Expr *> exprs;
            smt::Expr *vcSelfExpr = vcNodeSelfExpr(node);
            exprs.push_back(vcSelfExpr);
            MSGNode2smt(node, exprs);
            MSGNodePair curPair = std::make_pair(node, exprs);
            result.push_back(curPair);
        }

        return result;
    }

    smt::Expr *MSGManager::vcNodeSelfExpr(MSGNode *node) {
        if (node->isLeafNode()) {
            throwStrForValue("Error type of vc node", node);
        } else {
            smt::Expr *trueExpr = new BoolValue(true);
            smt::Expr *vcExpr = new Var(concatenate(MSGNODE_SMT_PREFIX, node->index), BoolSort());
            smt::Expr *vcNodeSelfExpr = new smt::Expr((*vcExpr) == (*trueExpr));
            return vcNodeSelfExpr;
        }
    }

    void MSGManager::throwStrForValue(string str, MSGNode *node) {
        if (node) {
            stringstream ss;
            ss << "MSGNode is leafNode:";
            if (node->isLeafNode()) {
                ss << "true ";
            } else {
                ss << "false ";
            }
            ss << "is VCNode:";
            if (node->isVCNode()) {
                ss << "true!";
            } else {
                ss << "false!";
            }
            throw (str + ss.str());
        } else {
            throw (str);
        }
    }

    MSGNode *MSGManager::expandSummaryNode(MSGSummaryNode &summaryNode) {
        // JK: DW TODO expand the summary node by symbolic execution
        MSGNode *msgNode = 0;
        // (a)->(b)
        if (summaryNode.funcName == "flip" && summaryNode.isRetValue) {
            VCGenerator vcGenerator;
            msgNode = vcGenerator.fake_1_expandSummaryNode(*msg, summaryNode);
        }

        // (b)->(c)
        if (summaryNode.funcName == "flip" && summaryNode.varName == "global2") {
            VCGenerator vcGenerator;
            msgNode = vcGenerator.fake_2_expandSummaryNode(*msg, summaryNode);
        }

        // (c)->(d)
        if (summaryNode.funcName == "init" && summaryNode.isRetValue) {
            VCGenerator vcGenerator;
            msgNode = vcGenerator.fake_3_expandSummaryNode(*msg, summaryNode);
        }

        return msgNode;
    }

    bool MSGManager::solve(MSGNodePair &msgNodePair) {
        auto solver = Z3RawSolver();
        std::vector<smt::Expr *> exprs = msgNodePair.second;
        for (std::vector<smt::Expr *>::iterator iter = exprs.begin(); iter != exprs.end(); iter++) {
            smt::Expr *tmp = *iter;
            solver.add(*tmp);
        }
//#if DEBUG_MSG
        std::cout << "MSGManager::solve SMT is:\n" << solver.getDebugInfo();
//#endif
        CheckResult checkResult = solver.checkSat();
        switch (checkResult) {
            case CheckResult::UNKNOWN:
                std::cout << "MSGManager::solve z3 failed!" << std::endl;
                this->verificationResult = UNKNOWN;
                return true;
            case CheckResult::UNSAT:
                std::cout << "MSGManager::solve smt unsat!" << std::endl;
                this->verificationResult = TRUE;
                return true;
            case CheckResult::SAT:
                std::cout << "MSGManager::solve smt sat!" << std::endl;
                // falsifying assignment is found,values assigned to nodes
                valueMSGNode(solver.getModel());
                this->verificationResult = FALSE;
                return false;
        }
    }

    /**
     * As is usual for graph traversal, visited nodes
     * are marked during traversal to avoid re-visiting nodes
     *
     * @param msgNodePair
     * @return
     */
    bool MSGManager::refine(MSGNodePair &msgNodePair) {
        MSGNode *msgNode = msgNodePair.first;
        if (msgNode->hasVisited) {
            return false;
        } else {
            msgNode->hasVisited = true;
        }

        if (MSGSummaryNode *summaryNode = dynamic_cast<MSGSummaryNode *>(msgNode)) {
            MSGNode *node = expandSummaryNode(*summaryNode);
            std::vector<smt::Expr *> exprs;
            MSGNode2smt(node, exprs);
            msgNodePair.first = node;
            msgNodePair.second = exprs;
            return true;
        } else if (msgNode->isLeafNode()) {
            return false;
        } else {
            if (MSGBinaryOPNode *binaryNode = dynamic_cast<MSGBinaryOPNode *>(msgNode)) {

                MSGNode *leftNode = binaryNode->left;
                MSGNode *rightNode = binaryNode->right;
                MSGOperator op = binaryNode->getMSGOperator();
                std::vector<smt::Expr *> exprs;
                MSGNodePair curPair;
                switch (op) {
                    case OR:
                        if (leftNode->smtValue == "true") {
                            if (MSGSummaryNode *leftSumNode = dynamic_cast<MSGSummaryNode *>(leftNode)) {
                                MSGNode2smt(leftNode, exprs);
                                curPair = std::make_pair(leftNode, exprs);
                                return refine(curPair);
                            }
                        } else if (rightNode->smtValue == "true") {
                            if (MSGSummaryNode *rightSumNode = dynamic_cast<MSGSummaryNode *>(rightNode)) {
                                MSGNode2smt(rightNode, exprs);
                                curPair = std::make_pair(rightNode, exprs);
                                return refine(curPair);
                            }
                        }
                        break;
                    case AND:
                        if (leftNode->smtValue == "false") {
                            if (MSGSummaryNode *leftSumNode = dynamic_cast<MSGSummaryNode *>(leftNode)) {
                                MSGNode2smt(leftNode, exprs);
                                curPair = std::make_pair(leftNode, exprs);
                                return refine(curPair);
                            }
                        } else if (rightNode->smtValue == "false") {
                            if (MSGSummaryNode *rightSumNode = dynamic_cast<MSGSummaryNode *>(rightNode)) {
                                MSGNode2smt(rightNode, exprs);
                                curPair = std::make_pair(rightNode, exprs);
                                return refine(curPair);
                            }
                        }
                        break;
                    case MULTIPLY:
                        if (leftNode->smtValue == "0") {
                            if (MSGSummaryNode *leftSumNode = dynamic_cast<MSGSummaryNode *>(leftNode)) {
                                MSGNode2smt(leftNode, exprs);
                                curPair = std::make_pair(leftNode, exprs);
                                return refine(curPair);
                            }
                        } else if (rightNode->smtValue == "0") {
                            if (MSGSummaryNode *rightSumNode = dynamic_cast<MSGSummaryNode *>(rightNode)) {
                                MSGNode2smt(rightNode, exprs);
                                curPair = std::make_pair(rightNode, exprs);
                                return refine(curPair);
                            }
                        }
                    default:
                        /**
                         *  TODO support more operator
                         */
                        break;
                }
                if (MSGSummaryNode *leftSumNode = dynamic_cast<MSGSummaryNode *>(leftNode)) {
                    MSGNode2smt(leftNode, exprs);
                    curPair = std::make_pair(leftNode, exprs);
                    return refine(curPair);
                } else if (MSGSummaryNode *rightSumNode = dynamic_cast<MSGSummaryNode *>(rightNode)) {
                    MSGNode2smt(rightNode, exprs);
                    curPair = std::make_pair(rightNode, exprs);
                    return refine(curPair);
                }
                return false; // can't be refined
            } else if (MSGITEOPNode *iteNode = dynamic_cast<MSGITEOPNode *>(msgNode)) {
                MSGNode *condNode = iteNode->condNode;
                MSGNode *trueNode = iteNode->trueNode;
                MSGNode *falseNode = iteNode->falseNode;
                std::vector<smt::Expr *> exprs;
                MSGNodePair curPair;
                if (condNode->smtValue == "true") {
                    if (MSGSummaryNode *trueSumNode = dynamic_cast<MSGSummaryNode *>(trueNode)) {
                        MSGNode2smt(trueNode, exprs);
                        curPair = std::make_pair(trueNode, exprs);
                        return refine(curPair);
                    }
                } else {
                    if (MSGSummaryNode *condSumNode = dynamic_cast<MSGSummaryNode *>(condNode)) {
                        MSGNode2smt(condNode, exprs);
                        curPair = std::make_pair(condNode, exprs);
                        return refine(curPair);
                    }
                }
                if (MSGSummaryNode *condSumNode = dynamic_cast<MSGSummaryNode *>(condNode)) {
                    MSGNode2smt(condNode, exprs);
                    curPair = std::make_pair(condNode, exprs);
                    return refine(curPair);
                } else if (MSGSummaryNode *trueSumNode = dynamic_cast<MSGSummaryNode *>(trueNode)) {
                    MSGNode2smt(trueNode, exprs);
                    curPair = std::make_pair(trueNode, exprs);
                    return refine(curPair);
                } else if (MSGSummaryNode *falseNode = dynamic_cast<MSGSummaryNode *>(falseNode)) {
                    MSGNode2smt(falseNode, exprs);
                    curPair = std::make_pair(falseNode, exprs);
                    return refine(curPair);
                } else {
                    return false; // can't be refined
                }
            } else if (MSGUnaryOPNode *unaryNode = dynamic_cast<MSGUnaryOPNode *>(msgNode)) {
                std::vector<smt::Expr *> exprs;
                MSGNode *operandNode = unaryNode->operatorNode;
                if (MSGSummaryNode *sumNode = dynamic_cast<MSGSummaryNode *>(operandNode)) {
                    MSGNode2smt(operandNode, exprs);
                    MSGNodePair curPair = std::make_pair(operandNode, exprs);
                    return refine(curPair);
                } else {
                    return false;
                }
            } else if (MSGVCNode *vcNode = dynamic_cast<MSGVCNode *>(msgNode)) {
                MSGNode *leftNode = vcNode->leftNode;
                MSGNode *rightNode = vcNode->rightNode;
                std::vector<smt::Expr *> exprs;
                MSGNodePair curPair;
                if (leftNode->smtValue == "false") {
                    MSGNode2smt(leftNode, exprs);
                    curPair = std::make_pair(leftNode, exprs);
                } else if (rightNode->smtValue == "true") {
                    MSGNode2smt(rightNode, exprs);
                    curPair = std::make_pair(rightNode, exprs);
                } else {
                    // refine leftNode by default
                    MSGNode2smt(leftNode, exprs);
                    curPair = std::make_pair(leftNode, exprs);
                }
                return refine(curPair);
            } else {
                throw ("[MSGManager.cpp refine unsupport msgNode type!]");
            }
            return false;
        }
    }

    void MSGManager::valueMSGNode(smt::Model model) {
        std::vector<MSGNode *> allNodes = msg->getAllNodes();
        for (std::vector<MSGNode *>::iterator iter = allNodes.begin(); iter != allNodes.end(); iter++) {
            MSGNode *node = *iter;
            node->smtValue = model[concatenate(MSGNODE_SMT_PREFIX,
                                               node->index)]; // the smt expression was named like smt_${index}
        }
    }

    void MSGManager::abstractionCheckingRefinementLoop() {
        //std::cout << std::endl;
        std::cout << "MSGManager::abstractionCheckingRefinementLoop starts!" << std::endl;
#if DEBUG_MSG
        std::cout<< "MSG:" << std::endl<<this->msg->toString()<<std::endl;
#endif

        bool valid = true;
        std::vector<MSGNodePair> vcFormulas = MSG2smt(); // encode VCs in the maximally-shared graph
        for (std::vector<MSGNodePair>::iterator iter = vcFormulas.begin(); iter != vcFormulas.end(); iter++) {
            while (!solve(*iter)) { // solve returns false if a solution (falsifying assignment) is found
                if (!refine((*iter))) {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                break;
            }
        }

        std::cout << "MSGManager::abstractionCheckingRefinementLoop ends!" << std::endl;
        switch (this->verificationResult) {
            case TRUE:
                std::cout << "TRUE" << std::endl;
                this->saManager->generateCorrectnessWitness();
                break;
            case FALSE:
                std::cout << "FALSE" << std::endl;
                this->saManager->generateViolationWitness();
                // DW: if found an error, you can simply quit
                exit(0);
                break;
            case UNKNOWN:
                std::cout << "UNKNOWN" << std::endl;
                break;
        }
    }

}
