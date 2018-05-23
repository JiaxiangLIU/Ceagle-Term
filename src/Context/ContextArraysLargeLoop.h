#ifndef CONTEXT_ARRAYS_LARGE_LOOP_H
#define CONTEXT_ARRAYS_LARGE_LOOP_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextArraysLargeLoop : public Context {
public:
	ContextArraysLargeLoop() {}
	void parse(std::string file);
};

}

#endif
