#include "ContextECA.h"

#include <regex>

void ceagle::ContextECA::parse(std::string fileContent) {
    std::regex rInput(R"(input\s*=\s*__VERIFIER_nondet_int\(\);)");
    std::regex rLimit(R"(if\s*\(\(input\s*!=\s*\d+\)(\s*&&\s*\(input\s*!=\s*\d+\)){0,9}\)\s*return\s*.*;)");
    auto location = fileContent.find("int main");
    std::smatch match;
    if (location != std::string::npos) {
        auto m = fileContent.substr(location + 8);
        if (std::regex_search(m, match, rInput)) {
            auto s = match[0].str();
            if (std::regex_search(m, match, rLimit)) {
                this->m_result.matched = true;
            }
        } else {
            this->m_result.matched = false;
            this->m_result.message.push_back(fileContent);
        }
    } else {
        this->m_result.matched = false;
    }
}
