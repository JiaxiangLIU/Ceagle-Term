#ifndef SLICING_FUNCTION_PREPROCESSOR_H
#define SLICING_FUNCTION_PREPROCESSOR_H

#include "Preprocessor.h"

using namespace llvm;

namespace ceagle {
	
class SlicingFunctionPreprocessor : public Preprocessor {
    std::vector<Function *> m_functionsNeccessary;
    std::string m_targetFunctionName;

    // visit times counting
    std::vector<Function *> m_functionsVisited;
    std::vector<Function *> m_functionsToDelete;
    bool visitFunction(Function *f);
    void removeFunction(Function *f);
    void removeFunctions(std::vector<Function *> vf);
    void outputFunctions(std::vector<Function *> vf);
public:
	SlicingFunctionPreprocessor(std::string functionName) : Preprocessor() {
        this->m_targetFunctionName = functionName;
	}
	virtual void preprocess(Module& module);
};
	
}

#endif
