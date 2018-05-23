#ifndef CONTEXT_NO_NONDET_H
#define CONTEXT_NO_NONDET_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextNoNondet : public Context {
public:
    virtual void parse(std::string) override;
};

}

#endif
