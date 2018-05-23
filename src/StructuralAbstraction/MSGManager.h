#ifndef SV_CEAGLE_MSGMANAGER_H
#define SV_CEAGLE_MSGMANAGER_H

#include <vector>
#include "SMT/Middleware/AST.h"
#include "MSG.h"
#include "SMT/Middleware/Model.h"
#include "SMT/Middleware/Z3RawSolver.h"

using std::string;
using namespace ceagle::smt;

namespace ceagle {

    typedef std::pair<ceagle::MSGNode *, std::vector<ceagle::smt::Expr *>> MSGNodePair;
    static const std::string MSGNODE_SMT_PREFIX = "smt_";

    enum VerificationResult{
        TRUE,FALSE,UNKNOWN
    };

    /**
     * MSG releated interface
    */
    class SAManager;
    class MSGManager {
    public:

        MSG *msg;

        SAManager *saManager;

        VerificationResult verificationResult = UNKNOWN;

        // DW: default msg creation
        MSGManager() {
            this->msg = new MSG();
        }

        MSGManager(MSG *m) : msg(m) {}

        MSGManager(SAManager *manager) {
            this->msg = new MSG();
            this->saManager = manager;
        }

        /**
         * MSG to SMT formula
         * @param msg
         * @return
         */
        std::vector<MSGNodePair> MSG2smt();

        /**
         * return expr :(vc==true)
         *
         * @param node
         * @return
         */
        smt::Expr *vcNodeSelfExpr(MSGNode *node);

        void throwStrForValue(std::string str, MSGNode *node = 0);

        void MSGNode2smt(MSGNode *msgNode, std::vector<smt::Expr *> &exprs);

        smt::Expr *MSGLeafNode2smt(MSGNode *msgNode);

        MSGNode *expandSummaryNode(MSGSummaryNode &summaryNode);

        bool solve(MSGNodePair &msgNodePair);

        bool refine(MSGNodePair &msgNodePair);

        /**
         * assign the model values to nodes
         *
         * @param msgNode
         * @param model
         */
        void valueMSGNode(smt::Model model);

        /**
         * Main abstaction-checking-refinement loop
         *
         * @param msg
         */
        void abstractionCheckingRefinementLoop();
    };
}


#endif //SV_CEAGLE_MSGMANAGER_H
