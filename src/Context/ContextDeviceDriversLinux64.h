#ifndef CONTEXT_DEVICE_DRIVERS_LINUX_64_H
#define CONTEXT_DEVICE_DRIVERS_LINUX_64_H

#include "Context.h"
#include <string>

namespace ceagle {

class ContextDeviceDriversLinux64 : public Context {
public:
	ContextDeviceDriversLinux64() {}
	void parse(std::string file);
};

}

#endif
