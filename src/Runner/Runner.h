#ifndef RUNNER_H 
#define RUNNER_H

#include <vector>
#include <string>

namespace ceagle {

struct RunnerResult {
    bool matched;
    std::vector<std::string> message;
    int status;
};

class Runner {
public:
    RunnerResult m_result;
    Runner() {this->m_result.matched = false; this->m_result.status = 0;}
    virtual void run(std::string rawName, std::string file) {}
    virtual RunnerResult getResult() { return m_result; }
    virtual ~Runner() = default;
};

}

#endif
