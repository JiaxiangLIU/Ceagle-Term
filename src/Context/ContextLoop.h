#ifndef CONTEXT_LOOP_H
#define CONTEXT_LOOP_H

#include <string>
#include "Context.h"

namespace ceagle {

class ContextLoop : public Context {
public:
    void parse(std::string) override;
};

}

#endif
