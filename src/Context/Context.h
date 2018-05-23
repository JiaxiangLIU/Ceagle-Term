#ifndef CONTEXT_H
#define CONTEXT_H 

#include <vector>
#include <string>

#include "debug.h"

namespace ceagle {

struct ContextResult {
    bool matched = false;
    std::vector<std::string> message;
    int status;
};

class Context {
public:
    ContextResult m_result;
	virtual void parse(std::string file) {this->m_result.matched = false;}
    virtual ContextResult getResult() { return m_result; }
    virtual ~Context() = default;
};

}

#endif
