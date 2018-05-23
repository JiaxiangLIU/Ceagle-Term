#ifndef NAMING_PREPROCESSOR_H
#define NAMING_PREPROCESSOR_H

#include "Preprocessor.h"

using llvm::BasicBlock;

namespace ceagle {
	
class NamingPreprocessor : public Preprocessor {
public:
	NamingPreprocessor() : Preprocessor() {
	}
	virtual void preprocess(Module& module);
};
	
}

#endif
