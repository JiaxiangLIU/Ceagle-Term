#ifndef CONTEXT_LDV_MALLOC_H
#define CONTEXT_LDV_MALLOC_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextLdvMalloc : public Context {
public:
    virtual void parse(std::string) override;
}; // class ContextLdvMalloc

} // namespace ceagle

#endif
