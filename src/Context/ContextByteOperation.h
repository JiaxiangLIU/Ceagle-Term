#ifndef CONTEXT_BYTE_OPERATION_H
#define CONTEXT_BYTE_OPERATION_H

#include <string>
#include "Context.h"

namespace ceagle {

class ContextByteOperation : public Context {
public:
    virtual void parse(std::string file) override;
};

};

#endif
