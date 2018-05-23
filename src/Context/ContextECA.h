#ifndef SV_CEAGLE_CONTEXT_ECA_H
#define SV_CEAGLE_CONTEXT_ECA_H

#include "Context/Context.h"

namespace ceagle {

class ContextECA : public Context {
public:
    virtual void parse(std::string) override;;
};

}


#endif //SV_CEAGLE_CONTEXTECA_H
