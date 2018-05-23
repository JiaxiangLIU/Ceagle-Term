#ifndef TRIM_PREPROCESSOR_H
#define TRIM_PREPROCESSOR_H

#include "Preprocess/Preprocessor.h"
#include <string>
#include <vector>
#include <map>

namespace ceagle {

enum class Status {
    NEW = 0,
    VISITING,
    VISITED
};

// Trim blocks that cannot reach targets.
// This only preprocess main function.

class TrimPreprocessor : public Preprocessor {
    class TrimPreprocessorImpl;
    std::unique_ptr<TrimPreprocessorImpl> impl;
public:
    TrimPreprocessor(std::vector<std::string> targets);
    TrimPreprocessor(TrimPreprocessor && other);
    virtual void preprocess(llvm::Module& module);
    ~TrimPreprocessor();
};

}

#endif
