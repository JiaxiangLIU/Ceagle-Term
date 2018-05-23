#include "RunnerManager.h"

#include <cstdarg>
#include <fstream>
#include "RunnerArraysLargeLoop.h"
#include "RunnerNoNondet.h"

namespace ceagle {

template<typename T> Runner * createRunner() {return new T;}

// np: number of names
RunnerManager::RunnerManager(int np, ... ) {
    // initialize map
    this->m_map[RN_ArraysLargeLoop] = &createRunner<RunnerArraysLargeLoop>;
    this->m_map[RN_NoNondet] = &createRunner<RunnerNoNondet>;

    va_list vl;
    va_start(vl, np);
    for (int i = 0; i < np; i++) {
        RunnerName p = (RunnerName)va_arg(vl, int);
        this->m_names.push_back(p);
    }
    va_end(vl);
}

void RunnerManager::run(std::string rawName, std::string file) {
    for (int i = 0; i < this->m_names.size(); i++) {
        RunnerName p = this->m_names[i];
        // create runner and let it run
        Runner *c = this->m_map[p]();
        c->run(rawName, file);
        RunnerResult cr = c->getResult();
        this->m_results.insert(std::pair<RunnerName, RunnerResult>(p, cr));
    }
}

RunnerResult RunnerManager::getResult(RunnerName cp) {
    return this->m_results[cp];
}

}
