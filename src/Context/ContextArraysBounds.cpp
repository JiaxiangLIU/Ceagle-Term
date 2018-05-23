#include "ContextArraysBounds.h"

#include <iostream>

namespace ceagle {

void ContextArraysBounds::parse(std::string file) {
#if DEBUG_CONTEXT
    std::cout << "ContextArraysBounds parsing ..." << std::endl;
    //std::cout << "file is " << std::endl << file << "---" << std::endl;
#endif

    // message are number of file
#if DEBUG_CONTEXT
    std::cout << "ContextArraysBounds found "  << this->m_result.matched << " " << std::endl;
#endif
}

}
