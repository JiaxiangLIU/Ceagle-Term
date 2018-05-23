#include <string>
#include <regex>
#include "ContextModuleAnalysis.h"

namespace ceagle {

void ContextModuleAnalysis::parse(std::string fileContent) {
    std::regex r(R"(__VERIFIER_assert\s*\((.*!=\d+)U?\)\s*;)");
    std::regex r2(R"(__VERIFIER_assert\s*\((.*==\d+)U?\)\s*;)");
    std::smatch match;
    std::smatch match2;
    if (!std::regex_search(fileContent, match2, r2) && std::regex_search(fileContent, match, r)) {
        this->m_result.matched = true;
        this->m_result.message.push_back(match[1]);
    }
}

}
