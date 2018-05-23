#include "VCGenerator.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/Support/FormattedStream.h>

#include "MSG.h"

namespace ceagle {

    MSG *VCGenerator::generateVCOfFunction(Function *function) {
        return NULL;
    }

    /**
     * Paper 07' Fig.2. (a)->(b)
     *
     * @param msg
     * @param summaryNode
     * @return
     */
    MSGNode *VCGenerator::fake_1_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode) {
        MSGBinaryOPNode *lessNode = new MSGBinaryOPNode();
        lessNode->msgOperator = MSGOperator::LESS;
        lessNode->childtt = MSGExpressionType::makeInt32Type();
        MSGVCNode *vcNode = dynamic_cast<MSGVCNode *>(msg.getAllNodes()[4]);
        vcNode->leftNode = lessNode;
        lessNode->right = msg.getAllNodes()[2];
        msg.addNode(lessNode);

        MSGSummaryNode *summaryNode2 = new MSGSummaryNode();
        summaryNode2->funcName = "init";
        summaryNode2->isRetValue = true;
        msg.addNode(summaryNode2);

        lessNode->left = summaryNode2;

        return lessNode;
    }

    MSGNode *VCGenerator::fake_2_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode) {
        MSGBinaryOPNode *parentNode = dynamic_cast<MSGBinaryOPNode *>(msg.getAllNodes()[summaryNode.parentsIndex[0]]);

        MSGITEOPNode *I1 = new MSGITEOPNode();
        I1->index = msg.getAllNodes().size();
        msg.getAllNodes().push_back(I1);
        parentNode->left = I1;

        MSGBinaryOPNode *lessNode = new MSGBinaryOPNode();
        lessNode->index = msg.getAllNodes().size();
        msg.getAllNodes().push_back(lessNode);
        lessNode->msgOperator = MSGOperator::LESS;
        lessNode->childtt = MSGExpressionType::makeInt32Type();
        I1->condNode = lessNode;

        MSGSummaryNode *summaryNode1 = new MSGSummaryNode();
        summaryNode1->funcName = "init";
        summaryNode1->isRetValue = true;
        summaryNode1->index = msg.getAllNodes().size();
        msg.getAllNodes().push_back(summaryNode1);
        I1->falseNode = summaryNode1;

        lessNode->left = summaryNode1;
        lessNode->right = msg.getAllNodes()[2];

        MSGUnaryOPNode *negaNode = new MSGUnaryOPNode();
        negaNode->msgOperator = MSGOperator::MINUS;
        negaNode->operatorNode = summaryNode1;
        negaNode->it = MSGExpressionType(MSGVarType::INT, "32");
        negaNode->index = msg.getAllNodes().size();
        msg.getAllNodes().push_back(negaNode);

        return I1;
    }

    MSGNode *VCGenerator::fake_3_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode) {
        MSGBinaryOPNode *parentNode = dynamic_cast<MSGBinaryOPNode *>(msg.getAllNodes()[summaryNode.parentsIndex[0]]);

        MSGVariableNode *variableNode = new MSGVariableNode("trivial");
        variableNode->it = MSGExpressionType(MSGVarType::INT, "32");
        variableNode->index = msg.getAllNodes().size();
        msg.getAllNodes().push_back(variableNode);

        parentNode->left = variableNode;

        return variableNode;
    }
}
