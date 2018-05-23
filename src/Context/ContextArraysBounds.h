#ifndef CONTEXT_ARRAYS_BOUNDS_H
#define CONTEXT_ARRAYS_BOUNDS_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextArraysBounds : public Context {
public:
	ContextArraysBounds() {}
	void parse(std::string file);
};

}

#endif
