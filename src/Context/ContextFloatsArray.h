#ifndef CONTEXT_FLOATS_ARRAY_H
#define CONTEXT_FLOATS_ARRAY_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextFloatsArray : public Context {
public:
	ContextFloatsArray() {}
	void parse(std::string file);
};

}

#endif
