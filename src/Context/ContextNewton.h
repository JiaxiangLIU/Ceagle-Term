#ifndef CONTEXT_NEWTON_H
#define CONTEXT_NEWTON_H 

#include "Context.h"
#include <string>

namespace ceagle {

class ContextNewton : public Context {
public:
	ContextNewton() {}
	void parse(std::string file);
};

}

#endif
