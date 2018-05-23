#include "ContextManager.h"

#include <cstdarg>
#include <fstream>
#include <map>
#include <string>
#include "ContextNewton.h"
#include "ContextFloatsArray.h"
#include "ContextArraysLargeLoop.h"
#include "ContextArraysBounds.h"
#include "ContextDeviceDriversLinux64.h"
#include "ContextByteOperation.h"
#include "ContextGCD.h"
#include "ContextModuleAnalysis.h"
#include "ContextLoop.h"
#include "ContextFloat.h"
#include "ContextNoNondet.h"
#include "ContextConcurrency.h"
#include "ContextLdvMalloc.h"
#include "ContextECA.h"
#include "ContextElevator.h"

namespace ceagle {

template<typename T> Context * createInstance() {return new T;}
//map[CP_Floats_Array] = &createInstance<DerivedB>;

// np: number of parsers
ContextManager::ContextManager(int np, ... ) {
    // initialize map
    this->m_map[CP_Newton] = &createInstance<ContextNewton>;
    this->m_map[CP_FloatsArray] = &createInstance<ContextFloatsArray>;
    this->m_map[CP_ArraysLargeLoop] = &createInstance<ContextArraysLargeLoop>;
    this->m_map[CP_ArraysBounds] = &createInstance<ContextArraysBounds>;
    this->m_map[CP_DeviceDriversLinux64] = &createInstance<ContextDeviceDriversLinux64>;
    this->m_map[CP_ByteOperation] = &createInstance<ContextByteOperation>;
    this->m_map[CP_GCD] = &createInstance<ContextGCD>;
    this->m_map[CP_ModuleAnalysis] = &createInstance<ContextModuleAnalysis>;
    this->m_map[CP_LOOP] = &createInstance<ContextLoop>;
    this->m_map[CP_FLOAT] = &createInstance<ContextFloat>;
    this->m_map[CP_NoNondet] = &createInstance<ContextNoNondet>;
    this->m_map[CP_Concurrency] = &createInstance<ContextConcurrency>;
    this->m_map[CP_LdvMalloc] = &createInstance<ContextLdvMalloc>;
    this->m_map[CP_ECA] = &createInstance<ContextECA>;
    this->m_map[CP_Elevator] = &createInstance<ContextElevator>;

    va_list vl;
    va_start(vl, np);
    for (int i = 0; i < np; i++) {
        ContextParser p = (ContextParser)va_arg(vl, int);
        this->m_parsers.push_back(p);
    }
    va_end(vl);
}

void ContextManager::parse(std::string fn) {
    std::ifstream ifs(fn);
    std::string file((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    for (int i = 0; i < this->m_parsers.size(); i++) {
        ContextParser p = this->m_parsers[i];
        /// create parser and let it parse
        Context *c = this->m_map[p]();
        c->parse(file);
        ContextResult cr = c->getResult();
        //this->m_results.insert(std::pair<ContextParser, ContextResult>(p, cr));

        // here, we do not have to find all results, just store the most precious one
        if (cr.matched) {
            this->m_matchedParser = p;
            this->m_matchedResult = cr;
            return;
        }
    }
    ifs.close();
}

ContextParser ContextManager::getMatchedParser() {
    return this->m_matchedParser;
}

ContextResult ContextManager::getMatchedResult() {
    return this->m_matchedResult;
}

std::map<ContextParser, std::string> cpMap = {
    {CP_Newton, "Newton"},
    {CP_FloatsArray, "FloatsArray"},
    {CP_ArraysLargeLoop, "ArraysLargeLoop"},
    {CP_ArraysBounds, "ArraysBounds"},
    {CP_DeviceDriversLinux64, "DeviceDriversLinux64"},
    {CP_ByteOperation, "ByteOperation"},
    {CP_GCD, "GCD"},
    {CP_ModuleAnalysis, "ModuleAnalysis"},
    {CP_LOOP, "LOOP"},
    {CP_FLOAT, "FLOAT"},
    {CP_NoNondet, "NoNondet"},
    {CP_Concurrency, "Concurrency"},
    {CP_LdvMalloc, "LdvMalloc"},
    {CP_ECA, "ECA"}
};

std::ostream& operator<<(std::ostream& out, ContextParser cp) {
    out << cpMap[cp];
    return out;
}

bool operator==(ContextParser cp, const std::string & str) {
    return cpMap[cp] == str;
}
bool operator==(const std::string & str, ContextParser cp) {
    return cp == str;
}

} // end namespace ceagle
