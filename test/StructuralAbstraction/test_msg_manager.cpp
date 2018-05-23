#include "StructuralAbstraction/MSGManager.h"
#include "SMT/Middleware/Z3RawSolver.h"

using namespace ceagle;
using namespace std;
using namespace ceagle::smt;

class MSGManagerTest {

public:

    /**
     * example from paper '08 Scalable and Precise Extended Static Checking' Figure 2
     */
    void test_MSG2smt1() {
        MSG msg;

        MSGVariableNode *aNode = new MSGVariableNode("a");
        aNode->it = MSGExpressionType::makeInt32Type();
        msg.addNode(aNode);

        MSGConstantNode *zeroNode = new MSGConstantNode("0");
        zeroNode->it = MSGExpressionType::makeInt32Type();
        msg.addNode(zeroNode);

        MSGBinaryOPNode *greatNode = new MSGBinaryOPNode();
        greatNode->msgOperator = MSGOperator::GREAT;
        greatNode->it = MSGExpressionType::makeBoolType();
        greatNode->childtt = MSGExpressionType::makeInt32Type();
        greatNode->left = zeroNode;
        greatNode->right = aNode;
        msg.addNode(greatNode);

        MSGUnaryOPNode *negaNode = new MSGUnaryOPNode();
        negaNode->msgOperator = MSGOperator::MINUS;
        negaNode->operatorNode = aNode;
        negaNode->it = MSGExpressionType::makeInt32Type();
        negaNode->childtt = MSGExpressionType::makeInt32Type();
        msg.addNode(negaNode);

        MSGITEOPNode *I2 = new MSGITEOPNode();
        I2->condNode = greatNode;
        I2->trueNode = negaNode;
        I2->falseNode = aNode;
        I2->it = MSGExpressionType::makeInt32Type();
        I2->childtt = MSGExpressionType::makeInt32Type();
        msg.addNode(I2);

        MSGBinaryOPNode *disEquNode = new MSGBinaryOPNode();
        disEquNode->msgOperator = MSGOperator::DIS_EQU;
        disEquNode->left = zeroNode;
        disEquNode->right = I2;
        disEquNode->it = MSGExpressionType::makeBoolType();
        disEquNode->childtt = MSGExpressionType::makeInt32Type();
        msg.addNode(disEquNode);

        MSGVCNode *vcNode = new MSGVCNode();
        vcNode->leftNode = greatNode;
        vcNode->rightNode = disEquNode;
        vcNode->it = MSGExpressionType::makeBoolType();
        vcNode->childtt = MSGExpressionType::makeInt32Type();
        msg.addNode(vcNode, true);

        MSGVariableNode *gNode = new MSGVariableNode("glob");
        gNode->it = MSGExpressionType::makeInt32Type();
        msg.addNode(gNode);

        MSGBinaryOPNode *divideNode = new MSGBinaryOPNode();
        divideNode->msgOperator = DIVIDE;
        divideNode->left = gNode;
        divideNode->right = I2;
        divideNode->childtt = MSGExpressionType::makeInt32Type();
        divideNode->it = MSGExpressionType::makeInt32Type();
        msg.addNode(divideNode);

        MSGITEOPNode *I1 = new MSGITEOPNode();
        I1->condNode = greatNode;
        I1->trueNode = divideNode;
        I1->falseNode = gNode;
        I1->it = MSGExpressionType::makeBoolType();
        I1->childtt = MSGExpressionType::makeInt32Type();
        msg.addNode(I1);

        MSGManager manager(&msg);

        manager.abstractionCheckingRefinementLoop();
    }

    /**
     * example from paper '07 Structural Abstraction of Software Verification Conditions*' Fig.2.(a)
     */
    void test_MSG2smt2() {
        MSG msg;

        MSGSummaryNode *summaryNode1 = new MSGSummaryNode();
        summaryNode1->funcName = "flip";
        summaryNode1->isRetValue = true;
        summaryNode1->it = MSGExpressionType::makeBoolType();
        std::vector<int> sum1ParentIndex;
        sum1ParentIndex.push_back(4);
        summaryNode1->parentsIndex = sum1ParentIndex;
        msg.addNode(summaryNode1);

        MSGSummaryNode *summaryNode2 = new MSGSummaryNode();
        summaryNode2->funcName = "flip";
        summaryNode2->isRetValue = false;
        summaryNode2->it = MSGExpressionType::makeInt32Type();
        summaryNode2->varName = "global2";
        std::vector<int> sum2ParentIndex;
        sum2ParentIndex.push_back(3);
        summaryNode2->parentsIndex = sum2ParentIndex;
        msg.addNode(summaryNode2);

        MSGConstantNode *zeroNode = new MSGConstantNode("0");
        zeroNode->it = MSGExpressionType::makeInt32Type();
        std::vector<int> zeroParentIndex;
        zeroParentIndex.push_back(3);
        zeroNode->parentsIndex = zeroParentIndex;
        msg.addNode(zeroNode);

        MSGBinaryOPNode *disEquNode = new MSGBinaryOPNode();
        disEquNode->msgOperator = MSGOperator::DIS_EQU;
        disEquNode->left = summaryNode2;
        disEquNode->right = zeroNode;
        std::vector<int> disParentIndex;
        disParentIndex.push_back(4);
        disEquNode->parentsIndex = disParentIndex;
        msg.addNode(disEquNode);

        MSGVCNode *vcNode = new MSGVCNode();
        vcNode->leftNode = summaryNode1;
        vcNode->rightNode = disEquNode;
        msg.addNode(vcNode, true);

        MSGManager manager(&msg);
        manager.abstractionCheckingRefinementLoop();
    }

    void test_smt_expr() {
        auto solver = Z3RawSolver();
        Expr *flipRet = new Var("smt_0", BoolSort());
        Expr *flipGlobal2 = new Var("smt_1", IntSort());
        Expr *zero = new IntValue(0);
        Expr *disEqu = new Expr(!((*flipGlobal2) == (*zero)));
        Expr *vc = new Expr(!(*flipRet) || (*disEqu));
        std::vector<Expr *> exprs;
        exprs.push_back(disEqu);
        exprs.push_back(vc);
        for (std::vector<smt::Expr *>::iterator iter = exprs.begin(); iter != exprs.end(); iter++) {
            Expr *tmp = *iter;
            solver.add(*tmp);
        }
        std::cout << solver.getDebugInfo() << std::endl;

        CheckResult checkResult = solver.checkSat();
        switch (checkResult) {
            case CheckResult::UNKNOWN:
                std::cout << "MSGManager::solve unknown result!" << std::endl;
            case CheckResult::UNSAT:
                std::cout << "MSGManager::solve verification is true!" << std::endl;
            case CheckResult::SAT:
                std::cout << "MSGManager::solve verification is not true!" << std::endl;
                // falsifying assignment is found,values assigned to nodes
        }
    }


    void test_smt_expr1() {
        auto solver = Z3RawSolver();
        Expr *equ = new Var("smt_0", BoolSort());
        Expr *a = new Var("smt_1", IntSort());
        Expr *minus = new Var("smt_2", IntSort());
        Expr *result1 = new Expr((*equ) == ((*a) == (*minus)));
        Expr *multi = new Var("smt_3", IntSort());
        Expr *d = new Var("smt_4", IntSort());
        Expr *result2 = new Expr((*minus) == ((*multi) - (*d)));
        Expr *b = new Var("smt_5", IntSort());
        Expr *c = new Var("smt_6", IntSort());
        Expr *result3 = new Expr((*multi) == ((*b) * (*c)));
        solver.add(*result1);
        solver.add(*result2);
        solver.add(*result3);
        std::cout << solver.getDebugInfo() << std::endl;
    }
};


int main() {
    MSGManagerTest test;
//    test.test_smt_expr();
//    test.test_smt_expr1();
//    test.test_MSG2smt1();
    test.test_MSG2smt2();
    return 0;
}
