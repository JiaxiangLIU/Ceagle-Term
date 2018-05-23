#ifndef GENERATE_FUNCTION_CALL_PREPROCESSOR_H
#define GENERATE_FUNCTION_CALL_PREPROCESSOR_H

#include "Preprocessor.h"

using namespace llvm;

namespace ceagle {
	
class GenerateFunctionCallPreprocessor : public Preprocessor {
    std::string m_targetFunctionName;

    // visit times counting
    int m_maxVisitTimes = 10;
    std::vector<Function *> m_functionVisited;
    std::map<Function *, int> m_functionVisitTimes;
    void outputFunction(Function *f, int indent);
public:
	GenerateFunctionCallPreprocessor(std::string functionName) : Preprocessor() {
        this->m_targetFunctionName = functionName;
	}
	virtual void preprocess(Module& module);
};
	
}

#endif
