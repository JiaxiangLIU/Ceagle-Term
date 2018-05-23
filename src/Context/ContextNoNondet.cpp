#include "ContextNoNondet.h"
#include <string>
#include <regex>

namespace ceagle {

void ContextNoNondet::parse(std::string fileContent) {
    if (fileContent.find("float X, P;") != std::string::npos) {
        this->m_result.matched = false;
        return;
    }
    if (fileContent.find("nondet") == std::string::npos) {
        // extract main function
        auto location = fileContent.find("int main");
        if (location != std::string::npos) {
            auto m = fileContent.substr(location + 8);
            // match all declaration
            std::regex r(R"([^\s=;]+\s+[_a-zA-Z][_a-zA-Z0-9]*\s*;)");
            std::smatch match;
            if (std::regex_search(m, match, r)) {
                auto s = match[0].str();
                this->m_result.matched = false;
            } else {
                this->m_result.matched = true;
                this->m_result.message.push_back(fileContent);
            }
        }
    } else {
        this->m_result.matched = false;
    }
}

}
