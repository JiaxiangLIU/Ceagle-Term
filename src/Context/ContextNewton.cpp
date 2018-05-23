#include "ContextNewton.h"

#include <iostream>
#include "../Util/StringUtil.h"

namespace ceagle {

void ContextNewton::parse(std::string file) {
#if DEBUG_CONTEXT
    std::cout << "ContextNewton parsing ..." << std::endl;
#endif
    int nffp = 0, nidx = 0;
    // message are number of file
    this->m_result.message.push_back(std::to_string(nffp));
    this->m_result.message.push_back(std::to_string(nidx));
#if DEBUG_CONTEXT
    std::cout << "ContextNewton found "  << this->m_result.matched << " " << nffp << " " << nidx << std::endl;
#endif
}

}
