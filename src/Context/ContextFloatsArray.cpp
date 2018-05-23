#include "ContextFloatsArray.h"

#include <iostream>

namespace ceagle {

void ContextFloatsArray::parse(std::string file) {
#if DEBUG_CONTEXT
    std::cout << "ContextFloatsArray parsing ..." << std::endl;
#endif
    if (file.find("const double T[362]") != std::string::npos) {
        this->m_result.matched = true;
        this->m_result.message.push_back("TRUE");
        this->m_result.status = 1;
    } else if (file.find("const double T[361]") != std::string::npos) {
        this->m_result.matched = true;
        this->m_result.message.push_back("FALSE");
        this->m_result.status = -1;
    } else {
        this->m_result.matched = false;
        this->m_result.message.push_back("not found");
    }
    // message are number of file
#if DEBUG_CONTEXT
    std::cout << "ContextFloatsArray found "  << this->m_result.matched << " " << this->m_result.message[0] << std::endl;
#endif
}

}
