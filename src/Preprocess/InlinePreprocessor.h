#ifndef INLINE_PREPROCESSOR_H
#define INLINE_PREPROCESSOR_H

#define DEFAULT_INLINE_LABEL "CEAGLE_INLINE_WARNING"
#define DEFAULT_RECURSION_DEPTH 3

#include "Preprocessor.h"

#include <vector>

using llvm::BasicBlock;

namespace ceagle {

class InlinePreprocessor : public Preprocessor {
	std::string inlineWarningLabel;
	int maxRecursionDepth;
	std::vector<std::string> inlineWhitelist;
	void removeAllSuccessorsPhiUses(BasicBlock* block);
public:
	InlinePreprocessor(std::string label, int depth) : Preprocessor() {
		this->inlineWarningLabel = label;
		this->maxRecursionDepth = depth;
	}
	InlinePreprocessor(std::string label) : InlinePreprocessor(label, DEFAULT_RECURSION_DEPTH) {}
	InlinePreprocessor(int depth) : InlinePreprocessor(DEFAULT_INLINE_LABEL, depth) {}
	InlinePreprocessor() : InlinePreprocessor(DEFAULT_INLINE_LABEL, DEFAULT_RECURSION_DEPTH) {}
	InlinePreprocessor(std::string label, int depth, const std::vector<std::string> whitelist) : InlinePreprocessor(label, depth) {this->inlineWhitelist = whitelist;}
	InlinePreprocessor(std::string label, const std::vector<std::string> whitelist) : InlinePreprocessor(label, DEFAULT_RECURSION_DEPTH) {this->inlineWhitelist = whitelist;}
	InlinePreprocessor(int depth, const std::vector<std::string> whitelist) : InlinePreprocessor(DEFAULT_INLINE_LABEL, depth) {this->inlineWhitelist = whitelist;}
	InlinePreprocessor(const std::vector<std::string> whitelist) : InlinePreprocessor(DEFAULT_INLINE_LABEL, DEFAULT_RECURSION_DEPTH) {this->inlineWhitelist = whitelist;}
	void setRecursionDepth(int depth);
	virtual void preprocess(Module& module);
};

}

#endif