#ifndef RUNNER_ARRAYS_LARGE_LOOP_H
#define RUNNER_ARRAYS_LARGE_LOOP_H

#include "Runner.h"
#include <string>

namespace ceagle {

class RunnerArraysLargeLoop : public Runner {
public:
	RunnerArraysLargeLoop() {}
	void run(std::string rawName, std::string file);
};

}

#endif
