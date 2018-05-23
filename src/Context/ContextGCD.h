#ifndef CONTEXT_GCD_H
#define CONTEXT_GCD_H

#include <string>
#include "Context.h"

namespace ceagle {

class ContextGCD : public Context {
public:
    virtual void parse(std::string file) override;
};

}

#endif
