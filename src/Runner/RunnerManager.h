#ifndef RUNNER_MANAGER_H
#define RUNNER_MANAGER_H 

#include <vector>
#include <map>

#include "Runner.h"

namespace ceagle {

enum RunnerName {
    RN_ArraysLargeLoop,
    RN_NoNondet
};

typedef std::map<RunnerName, Runner*(*)()> RunnerMap;

class RunnerManager {
    RunnerMap m_map;
public:
    std::vector<RunnerName> m_names;
    std::map<RunnerName, RunnerResult> m_results;

	RunnerManager(int np, ...);
	void run(std::string rawName, std::string file);
    RunnerResult getResult(RunnerName cp);
};

}

#endif
