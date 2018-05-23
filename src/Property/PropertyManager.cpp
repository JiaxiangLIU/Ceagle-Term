#include "PropertyManager.h"

#include <fstream>

namespace ceagle {

void PropertyManager::parse(std::string fn) {
    std::ifstream ifs(fn);
    std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    // find properties
    if (file.find("call(__VERIFIER_error())") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_Error_Function_Unreachability);
    }
    if (file.find("G valid-free") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_Valid_Free);
    }
    if (file.find("G valid-deref") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_Valid_Dereference);
    }
    if (file.find("G valid-memtrack") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_Valid_Memory_Tracking);
    }
    if (file.find("overflow") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_No_Overflow);
    }
    if (file.find("F end") != std::string::npos) {
        this->m_propertiesFound.push_back(PT_SVCOMP_Termination);
    }
}

PropertyType PropertyManager::getProperty() {
    if (this->m_propertiesFound.size() > 0) {
        return this->m_propertiesFound[0];
    } else {
        return PT_Nothing;
    }
}

// human readable string
std::string PropertyManager::getPropertyString() {
    std::string re = "";
    switch (this->getProperty()) {
        case PT_Nothing:
            re = "[Nothing]";
            break;
        case PT_SVCOMP_Error_Function_Unreachability:
            re = "[CHECK( init(main()), LTL(G ! call(__VERIFIER_error())) )]";
            break;
        case PT_SVCOMP_Valid_Free:
            re = "[CHECK( init(main()), LTL(G valid-free) )]";
            break;
        case PT_SVCOMP_Valid_Dereference:
            re = "[CHECK( init(main()), LTL(G valid-deref) )]";
            break;
        case PT_SVCOMP_Valid_Memory_Tracking:
            re = "[CHECK( init(main()), LTL(G valid-memtrack) )]";
            break;
        case PT_SVCOMP_No_Overflow:
            re = "[CHECK( init(main()), LTL(G ! overflow) )]";
            break;
        case PT_SVCOMP_Termination:
            re = "[CHECK( init(main()), LTL(F end) )]";
            break;
        default:
            re = "[Error]";
            break;
    }
    return re;
}

}
