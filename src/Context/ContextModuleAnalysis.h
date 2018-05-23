#ifndef CONTEXT_MODULE_ANALYSIS_H
#define CONTEXT_MODULE_ANALYSIS_H

#include <string>
#include "Context.h"

namespace ceagle {

class ContextModuleAnalysis : public Context {
public:
    virtual void parse(std::string fileContent) override;
};

}

#endif
