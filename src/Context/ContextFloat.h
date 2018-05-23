#ifndef CONTEXT_FLOAT_H
#define CONTEXT_FLOAT_H

#include "Context.h"

#include <string>

namespace ceagle {

class ContextFloat : public Context {
public:
    virtual void parse(std::string) override;

    std::string removeExternFunctions(std::string fileContent);
};

}
#endif
