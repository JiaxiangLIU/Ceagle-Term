#include "ContextLdvMalloc.h"
#include <vector>

namespace ceagle {

void ContextLdvMalloc::parse(std::string fileContent) {
    if (fileContent.find("ldv_malloc") != std::string::npos) {
        this->m_result.matched = true;
    }
}

} // namespace ceagle
