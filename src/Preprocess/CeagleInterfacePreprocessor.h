#ifndef CEAGLE_INTERFACE_PREPROCESSOR_H
#define CEAGLE_INTERFACE_PREPROCESSOR_H

#define DEFAULT_ASSERT_FUNC_NAME "assert"
#define DEFAULT_ASSUME_FUNC_NAME "assume"
#define DEFAULT_ASSERT_LABEL "CEAGLE_ASSERT_FAIL"
#define DEFAULT_ASSUME_LABEL "CEAGLE_ASSUME_FAIL"

#include "Preprocessor.h"

using llvm::BasicBlock;

namespace ceagle {

class CeagleInterfacePreprocessor : public Preprocessor {
	std::string assertFuncName;
	std::string assumeFuncName;
	std::string assertFailLabel;
	std::string assumeFailLabel;
	void removeAllSuccessorsPhiUses(BasicBlock* block);
public:
	CeagleInterfacePreprocessor(std::string assertFuncName = DEFAULT_ASSERT_FUNC_NAME, std::string assumeFuncName = DEFAULT_ASSUME_FUNC_NAME, std::string assertFailLabel = DEFAULT_ASSERT_LABEL, std::string assumeFailLabel = DEFAULT_ASSUME_LABEL) : Preprocessor() {
		this->assertFuncName = assertFuncName;
		this->assumeFuncName = assumeFuncName;
		this->assertFailLabel = assertFailLabel;
		this->assumeFailLabel = assumeFailLabel;
	}
	virtual void preprocess(Module& module);
};

}

#endif