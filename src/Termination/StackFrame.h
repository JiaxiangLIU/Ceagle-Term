/*
 * StackFrame.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jiaxiang
 */

#ifndef SRC_TERMINATION_STACKFRAME_H_
#define SRC_TERMINATION_STACKFRAME_H_

#include "BasicTypes.h"

namespace ceagle {

class StackFrame {
public:
    ProgramCounter pc;
    LocalVars LVi;
    AllocationList ALi;
};

}

#endif /* SRC_TERMINATION_STACKFRAME_H_ */
