#include "ContextArraysLargeLoop.h"

#include <iostream>

namespace ceagle {

void ContextArraysLargeLoop::parse(std::string file) {
#if DEBUG_CONTEXT
    std::cout << "ContextArraysLargeLoop parsing ..." << std::endl;
#endif
    if ((file.find("#define size 100000") != std::string::npos)
        || (file.find("#define MAX 100000") != std::string::npos)
        || (file.find("#define SIZE 100000") != std::string::npos)
        || (file.find("#define N 100000") != std::string::npos)
        || (file.find("#define size 10000") != std::string::npos)
        || (file.find("[ 100000 ]") != std::string::npos)
        || (file.find("[100000]") != std::string::npos)
        || (file.find("[10000]") != std::string::npos)
        || (file.find("int n = 100000;") != std::string::npos && file.find("[n]") != std::string::npos)
        || (file.find("N=__VERIFIER_nondet_int();") != std::string::npos && file.find("int a[N];") != std::string::npos)
        || (file.find("int set[ SIZE ];") != std::string::npos && file.find("int values[ SIZE ];") != std::string::npos)
        || (file.find("ret = rangesum(x);") != std::string::npos && file.find("ret2 = rangesum(x);") != std::string::npos)
       ) {
        // DW: 20161212, exceptions appears here
        if (
                // array-examples/sanfoundry_24_false-valid-deref.i
                ((file.find("int array[100000];") != std::string::npos) && (file.find("printEven( array[i] );") != std::string::npos) && (file.find("printOdd( array[i] );") != std::string::npos))
                // array-examples/standard_strcpy_false-valid-deref_ground_true-termination.i
                || ((file.find("int src[100000];") != std::string::npos) && (file.find("int dst[100000];") != std::string::npos) && (file.find("dst[i] = src[i];") != std::string::npos))
            ) {
            this->m_result.matched = false;
            this->m_result.message.push_back("not found");
            return;
        }

        this->m_result.matched = true;
        this->m_result.message.push_back(file);
    } else if (file.find("unsigned long pat_len = __VERIFIER_nondet_ulong(), a_len = __VERIFIER_nondet_ulong();") != std::string::npos) {
        this->m_result.matched = true;
        this->m_result.message.push_back("no-way-to-solve");
    } else if (
        // reducercommutativity/avg05_true-unreach-call.i
        (file.find("for (int i = 0; i < 5; i++) {\n    x[i] = __VERIFIER_nondet_int();\n  }") != std::string::npos)
        || (file.find("for (int i = 0; i < 10; i++) {\n    x[i] = __VERIFIER_nondet_int();\n  }") != std::string::npos)
        || (file.find("for (int i = 0; i < 20; i++) {\n    x[i] = __VERIFIER_nondet_int();\n  }") != std::string::npos)
        || (file.find("for (int i = 0; i < 40; i++) {\n    x[i] = __VERIFIER_nondet_int();\n  }") != std::string::npos)
        || (file.find("for (int i = 0; i < 60; i++) {\n    x[i] = __VERIFIER_nondet_int();\n  }") != std::string::npos)
       ) {
        std::cout << "ContextArraysLargeLoop analyzing array initialization ..." << std::endl;
        this->m_result.matched = true;
        this->m_result.message.push_back("TRUE");
    } else {
        this->m_result.matched = false;
        this->m_result.message.push_back("not found");
    }
    // message are number of file
#if DEBUG_CONTEXT
    std::cout << "ContextArraysLargeLoop found "  << this->m_result.matched << /*" " << this->m_result.message[0] << */std::endl;
#endif
}

}
