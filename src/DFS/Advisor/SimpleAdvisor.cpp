#include "SimpleAdvisor.h"

#include <iostream>

namespace ceagle {

void SimpleAdvisor::dfsVisit(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsVisit(module, currentPath);
    std::cout << "SimpleAdvisor::dfsVisit" << std::endl;
}

NextChoice<P_BB> SimpleAdvisor::dfsNext(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsNext(module, currentPath);
    std::cout << "SimpleAdvisor::dfsNext" << std::endl;
    struct NextChoice<P_BB> nc;
    return nc;
}

BackwardPolicy SimpleAdvisor::dfsBackward(const Module& module, R_BBPATH currentPath) {
    Advisor<>::dfsBackward(module, currentPath);
    std::cout << "SimpleAdvisor::dfsBackward" << std::endl;
    return BackwardPolicy::BP_ALLOW;
}

}
