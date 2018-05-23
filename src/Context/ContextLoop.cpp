#include "ContextLoop.h"
#include <string>
#include <regex>

#include "DFS/Absref/AbstractState.h"

namespace ceagle {

void ContextLoop::parse(std::string fileContent) {
    std::regex r(R"(while|for)");
    std::smatch match;
    if (std::regex_search(fileContent, match, r)) {
        this->m_result.matched = true;
        AbstractState::sound = true;
    }
}

}

