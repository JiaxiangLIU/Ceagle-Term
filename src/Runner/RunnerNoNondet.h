#ifndef RUNNER_NO_NONDET_H
#define RUNNER_NO_NONDET_H

#include "Runner.h"
#include <string>

namespace ceagle {

class RunnerNoNondet : public Runner {
public:
    virtual void run(std::string, std::string) override;
};

} // end namespace ceagle

#endif
