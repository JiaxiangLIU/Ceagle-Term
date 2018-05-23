#include "ContextConcurrency.h"

#include <string>
#include <regex>

namespace ceagle {

void ContextConcurrency::parse(std::string fileContent) {
    std::regex r(R"((pthread_create))");
    std::smatch match;
    if (std::regex_search(fileContent, match, r)) {
        this->m_result.matched = true;
        this->m_result.message.push_back(match[1]);
    }
}

}
