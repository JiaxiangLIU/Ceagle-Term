#include "ContextGCD.h"

#include <string>

namespace ceagle {

void ContextGCD::parse(std::string fileContent) {
    auto findGCD = fileContent.find("gcd_test") != std::string::npos;
    if (findGCD) {
        this->m_result.matched = true;
    }
}

}
