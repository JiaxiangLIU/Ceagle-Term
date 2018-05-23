#ifndef CEAGLE_CONTEXT_ELEVATOR_H
#define CEAGLE_CONTEXT_ELEVATOR_H

#include "Context.h"

namespace ceagle {

class ContextElevator : public Context {
public:
    void parse(std::string file) override;
};

}


#endif //CEAGLE_CONTEXT_ELEVATOR_H
