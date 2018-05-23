#include <string>
#include "DFS/Advisor/Advisor.h"
#include "NaiveAdvisor.h"
#include "SimpleAdvisor.h"
#include "ArraysBoundsAdvisor.h"
#include "AbsrefAdvisor.h"
#include "TestAdvisor.h"

using namespace std;

namespace {
    enum class AdvisorType {
        Undefined,
        Naive,
        Simple,
        ArraysBounds,
        Absref,
        Test
    };
    static std::map<std::string, AdvisorType> advisorMap = {
        {"naive", AdvisorType::Naive},
        {"simple", AdvisorType::Simple},
        {"absref", AdvisorType::Absref},
        {"arraysbounds", AdvisorType::ArraysBounds},
        {"test", AdvisorType::Test}
    };
}

namespace ceagle {

Advisor<>* makeAdvisor(const std::string& name) {
    auto type = advisorMap[name];
    switch(type) {
        case AdvisorType::Naive:
            return new NaiveAdvisor();
        case AdvisorType::Simple:
            return new SimpleAdvisor();
        case AdvisorType::Absref:
            return new AbsrefAdvisor();
        case AdvisorType::ArraysBounds:
            return new ArraysBoundsAdvisor();
        case AdvisorType::Test:
            return new TestAdvisor();
        default:
            return new Advisor<>();
    }
}

}
