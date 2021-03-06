#if DEBUG

// modify following for enable/disable debug log, do not commit this file!!!
#define MODULE_DUMP 0
#define INTERACTIVE 1
#define CHECKDEBUG 1
#define TRDEBUG 1

// general advisor information
#define DEBUG_ADVISOR 1
// used in NaiveAdvisor only
#define DEBUG_ADVISOR_NAIVE 1
// path SMT generated by translators
#define DEBUG_PATH_SMT 1

#define DEBUG_CONTEXT 1
#define DEBUG_RUNNER 0
#define DEBUG_AST 0
#define DEBUG_MSG 1

#define DEBUG_SlicingFunctionPreprocessor 1

#else

#define MODULE_DUMP 0
#define INTERACTIVE 0
#define CHECKDEBUG 0
#define TRDEBUG 0

// general advisor information
#define DEBUG_ADVISOR 0
// used in NaiveAdvisor only
#define DEBUG_ADVISOR_NAIVE 0
// path SMT generated by translators
#define DEBUG_PATH_SMT 0

#define DEBUG_CONTEXT 0
#define DEBUG_RUNNER 0
#define DEBUG_AST 0
#define DEBUG_MSG 0

#define DEBUG_SlicingFunctionPreprocessor 0

#endif
