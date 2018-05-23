#include "MSG.h"

#include <iostream>
#include <sstream>
#include <z3.h>

using namespace ceagle;

namespace ceagle {

    std::string getTabsByDepth(int depth) {
        std::string result = "";
        for (int i = 0; i < depth; i++) {
            result += "\t";
        }
        return result;
    }

    void MSG::addNode(MSGNode *node, bool isVCNode) {
        // DW: also need to specify the index
        node->index = this->allNodes.size();
        this->allNodes.push_back(node);

        // DW: HJK's code are listed below
        if (isVCNode) {
            vcNodes.push_back(node);
            nonLeafNodes.push_back(node);
        } else {
            if (MSGSummaryNode *summaryNode = dynamic_cast<MSGSummaryNode *>(node)) {
                nonLeafNodes.push_back(node);
            } else if (MSGVariableNode *variableNode = dynamic_cast<MSGVariableNode *> (node)) {
                leafNodes.push_back(node);
            } else if (MSGConstantNode *constantNode = dynamic_cast<MSGConstantNode *>(node)) {
                leafNodes.push_back(node);
            } else if (MSGOPNode *opNode = dynamic_cast<MSGOPNode *>(node)) {
                std::cout << "MSG::addNode push back :" << node->toString() << std::endl;
                nonLeafNodes.push_back(node);
            }
        }
    }

//    const std::vector<MSGNode *> &MSG::getResultNodes() const {
//        return resultNodes;
//    }


    const std::vector<MSGNode *> &MSG::getLeafNodes() const {
        return leafNodes;
    }


    const std::vector<MSGNode *> &MSG::getNonLeafNodes() const {
        return nonLeafNodes;
    }


    std::vector<MSGNode *> &MSG::getAllNodes() {
        return allNodes;
    }


    const std::vector<MSGNode *> &MSG::getVcNodes() const {
        return vcNodes;
    }

    std::string MSG::toString() {
        std::string result = "";
        for (std::vector<MSGNode *>::const_iterator iter = vcNodes.cbegin(); iter != vcNodes.cend(); iter++) {
            int depth = 0;
            MSGNode *msgNode = *iter;
            constructMSGInfo(msgNode, result, depth);
        }
        return result;
    }

    void MSG::constructMSGInfo(MSGNode *node, std::string &result, int depth) {
        if (node->isLeafNode()) {
            result += getTabsByDepth(depth);
            if (MSGSummaryNode *summaryNode = dynamic_cast<MSGSummaryNode *>(node)) {
                result += summaryNode->toString();
            } else if (MSGVariableNode *variableNode = dynamic_cast<MSGVariableNode *>(node)) {
                result += variableNode->toString();
            } else if (MSGConstantNode *constantNode = dynamic_cast<MSGConstantNode *>(node)) {
                result += constantNode->toString();
            } else {
                throw ("[MSG.cpp constructMSGInfo] msgNode isn't leafNode");
            }
            result += "\n";
        } else {
            if (MSGVCNode *vcNode = dynamic_cast<MSGVCNode *>(node)) {
                result += (getTabsByDepth(depth) + vcNode->toString() + "\n");
                MSGNode *leftNode = vcNode->leftNode;
                MSGNode *rightNode = vcNode->rightNode;
                constructMSGInfo(leftNode, result, depth + 1);
                //result += "\n";
                //result += "[vc l n]";
                constructMSGInfo(rightNode, result, depth + 1);
                //result += "\n";
                //result += "[vc r n]";
            } else if (MSGUnaryOPNode *unaryNode = dynamic_cast<MSGUnaryOPNode *>(node)) {
                result += (getTabsByDepth(depth) + unaryNode->toString() + "\n");
                MSGNode *operandNode = unaryNode->operatorNode;
                constructMSGInfo(operandNode, result, depth + 1);
                //result += "\n";
                //result += "[unary n]";
            } else if (MSGBinaryOPNode *binaryNode = dynamic_cast<MSGBinaryOPNode *>(node)) {
                result += (getTabsByDepth(depth) + binaryNode->toString() + "\n");
                MSGNode *leftNode = binaryNode->left;
                MSGNode *rightNoe = binaryNode->right;
                constructMSGInfo(leftNode, result, depth + 1);
                //result += "\n";
                //result += "[binary l n]";
                constructMSGInfo(rightNoe, result, depth + 1);
                //result += "\n";
                //result += "[binary r n]";
            } else if (MSGITEOPNode *iteNode = dynamic_cast<MSGITEOPNode *>(node)) {
                result += (getTabsByDepth(depth) + iteNode->toString() + "\n");
                MSGNode *condNode = iteNode->condNode;
                MSGNode *trueNode = iteNode->trueNode;
                MSGNode *falseNode = iteNode->falseNode;
                constructMSGInfo(condNode, result, depth + 1);
                //result += "\n";
                //result += "[ite c n]";
                constructMSGInfo(trueNode, result, depth + 1);
                //result += "\n";
                //result += "[ite t n]";
                constructMSGInfo(falseNode, result, depth + 1);
                //result += "\n";
                //result += "[ite f n]";
            } else {
                throw ("[MSG.cpp constructMSGInfo] msgNode isn't nonleafNode");
            }
        }
    }
}
