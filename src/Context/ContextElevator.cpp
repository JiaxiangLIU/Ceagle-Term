//
// Created by Guang Chen on 18/12/2016.
//

#include "ContextElevator.h"

void ceagle::ContextElevator::parse(std::string file) {
    auto location = file.find("select_one");
    auto location2 = file.find("select_features");
    if (location != std::string::npos && location2 != std::string::npos) {
        this->m_result.matched = true;
    }
}
