#ifndef VC_GENERATOR_H
#define VC_GENERATOR_H

#include "llvm/IR/Module.h"

#include "MSG.h"

using namespace llvm;

namespace ceagle {

    class VCGenerator {
    public:
        VCGenerator() {
        }

        MSG *generateVCOfFunction(Function *function);

        //Expr *generateCallConditionOfFunction(Function *child, Function *parent);
        MSGNode *fake_1_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode);

        MSGNode *fake_2_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode);

        MSGNode *fake_3_expandSummaryNode(MSG &msg, MSGSummaryNode &summaryNode);
    };

}

#endif
