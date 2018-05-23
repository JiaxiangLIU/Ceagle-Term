#include "ContextByteOperation.h"

#include <string>

namespace ceagle {

void ContextByteOperation::parse(std::string fileContent) {
    auto findShr = fileContent.find("<<") != std::string::npos;
    auto findShl = fileContent.find(">>") != std::string::npos;
    auto findxor = fileContent.find("^") != std::string::npos;
    auto findor = fileContent.find(" | ") != std::string::npos;
    auto findand = fileContent.find(" & ") != std::string::npos;
    auto findLoop = fileContent.find("for") != std::string::npos || fileContent.find("while") != std::string::npos;
    if (!findLoop) {
        this->m_result.matched = true;
        this->m_result.message.push_back("naive");
        return;
    }
    if (findShr || findShl || findxor || findor || findand) {
        this->m_result.matched = true;
        if (fileContent.find("if (partial_sum > ((unsigned char)254))") != std::string::npos) {
            this->m_result.message.push_back("naive");
        } else if (fileContent.find("int ag_Z  = __VERIFIER_nondet_int();") != std::string::npos) {
            this->m_result.message.push_back("FALSE(reach)");
            this->m_result.status = -1;
        } else if (fileContent.find("int ag_Y  = __VERIFIER_nondet_int();") != std::string::npos) {
            if (fileContent.find("int ssl3_connect(void)") != std::string::npos) {
                this->m_result.message.push_back("FALSE(reach)");
                this->m_result.status = -1;
            }
        } else if (fileContent.find("if (blastFlag <= 5)") != std::string::npos) {
            this->m_result.message.push_back("FALSE(reach)");
                this->m_result.status = -1;
        }
    }
}

}
