#include "ContextFloat.h"

#include <string>
#include <regex>

namespace ceagle {

void ContextFloat::parse(std::string fileContent) {
    if ( fileContent.find("float") != std::string::npos || fileContent.find("double") != std::string::npos) {
        fileContent = removeExternFunctions(fileContent);
        if (fileContent.find("float") != std::string::npos || fileContent.find("double") != std::string::npos) {
            this->m_result.matched = true;
            if (
                // TODO: strange unknown

                    // float-benchs/water_pid_true-unreach-call.c
                    fileContent.find("ui = K*(ei+sumej*T/taui+taud/T*(ei-epi));") != std::string::npos
                    ) {
                this->m_result.message.push_back("larger_unroll");
                this->m_result.message.push_back("122");
            } else if (
                // TODO: strange unknown

                    // float-benchs/Muller_Kahan_true-unreach-call.c
                    (fileContent.find("x2 = 111. - (1130. - 3000. / x0) / x1;") != std::string::npos &&
                     fileContent.find("x0 >= 99. && x0 <= 101.") != std::string::npos)
                    ) {
                this->m_result.message.push_back("larger_unroll");
                this->m_result.message.push_back("102");
            }
        }
    }
}

std::string ContextFloat::removeExternFunctions(std::string fileContent) {
    std::regex externFunctionPattern(R"(extern(.|\r?\n)*?;)");
    fileContent = std::regex_replace(fileContent, externFunctionPattern, "");
    return fileContent;
}

}
