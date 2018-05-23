#ifndef SVCOMP_INTERFACE_PREPROCESSOR_H
#define SVCOMP_INTERFACE_PREPROCESSOR_H

#include "macros.h"

#include "Preprocessor.h"

using llvm::BasicBlock;

namespace ceagle {
	
class SVCompInterfacePreprocessor : public Preprocessor {
	std::string errorLabel;
	std::string assumeFailLabel;
	void removeAllSuccessorsPhiUses(BasicBlock* block);
public:
	SVCompInterfacePreprocessor(std::string errorLabel = DEFAULT_VERIFIER_ERROR_LABEL, std::string assumeFailLabel = DEFAULT_VERIFIER_ASSUME_LABEL) : Preprocessor() {
		this->errorLabel = errorLabel;
		this->assumeFailLabel = assumeFailLabel;
	}
	virtual void preprocess(Module& module);
};
	
}

#endif
