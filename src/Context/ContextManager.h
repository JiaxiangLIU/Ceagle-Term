#ifndef CONTEXT_MANAGER_H
#define CONTEXT_MANAGER_H

#include <vector>
#include <map>

#include "Context.h"

namespace ceagle {

enum ContextParser {
    CP_NONE,
    CP_Newton,
    CP_FloatsArray,
    CP_ArraysLargeLoop,
    CP_ArraysBounds,
    CP_DeviceDriversLinux64,
    CP_ByteOperation,
    CP_GCD,
    CP_ModuleAnalysis,
    CP_LOOP,
    CP_FLOAT,
    CP_NoNondet,
    CP_Concurrency,
    CP_LdvMalloc,
	CP_ECA,
	CP_Elevator
};

std::ostream& operator<<(std::ostream&, ContextParser);
bool operator==(ContextParser, const std::string &);
bool operator==(const std::string &, ContextParser);

typedef std::map<ContextParser, Context*(*)()> ParserMap;

class ContextManager {
    ParserMap m_map;
public:
    std::vector<ContextParser> m_parsers;
    std::map<ContextParser, ContextResult> m_results;
    ContextParser m_matchedParser;
    ContextResult m_matchedResult;

	ContextManager(int np, ...);
	void parse(std::string fn);
    ContextParser getMatchedParser();
    ContextResult getMatchedResult();
};

}

#endif
