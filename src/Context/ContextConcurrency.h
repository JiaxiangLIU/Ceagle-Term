#ifndef CONTEXT_CONCURRENCY_H
#define CONTEXT_CONCURRENCY_H

#include "Context.h"

#include <string>

namespace ceagle {

class ContextConcurrency : public Context {
public:
    virtual void parse(std::string) override;
}; // class ContextConcurrency

} // namespace ceagle

#endif
