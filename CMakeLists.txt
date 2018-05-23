cmake_minimum_required(VERSION 3.2)
project(sv-ceagle)
set(CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

if(COVERAGE)
include(CodeCoverage)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DDARWIN=1)
endif()

# set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-write-strings")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lclangTooling -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangEdit -lclangAST -lclangLex -lclangBasic")
# if need build with static, type cmake -DCXX_STATIC=ON ..
if(CXX_STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif(CXX_STATIC)
# debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug mode")
    add_definitions(-DDEBUG=1)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Release mode")
    add_definitions(-DDEBUG=0)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# find llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM_PACKAGE_VERSION: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR: ${LLVM_DIR}")
message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")
message(STATUS "LLVM_PATH: ${LLVM_PATH}")
message(STATUS "LLVM_CMAKE_DIR: ${LLVM_CMAKE_DIR}")
message(STATUS "LLVM_EXTERNAL_PROJECTS: ${LLVM_EXTERNAL_PROJECTS}")
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM_EXTERNAL_CLANG_SOURCE_DIR: ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}")
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core irreader transformutils scalaropts)
message(STATUS "llvm_libs: ${llvm_libs}")

# build settings
set(CEAGLE_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CEAGLE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CEAGLE_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

# libs
link_directories(${LLVM_DIR}/../../../lib/)

# includes
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(
    ${CEAGLE_PROJECT_DIR}/src
    ${CEAGLE_PROJECT_DIR}/include
    ${PROJECT_BINARY_DIR}
)
# clang tooling includes
include_directories(${LLVM_DIR}/../../../../tools/clang/include/)
include_directories(${LLVM_DIR}/../../../tools/clang/include/)

# hash
include_directories(${CEAGLE_PROJECT_DIR}/include/hashlib++)

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_SYSTEM: ${CMAKE_SYSTEM}")
if(UNIX AND NOT APPLE)
	if ("${CMAKE_SYSTEM_VERSION}" MATCHES "fc")
		message("Using Fedora hashlib++")
		link_libraries("${CEAGLE_PROJECT_DIR}/lib/fedora/libhl++.a")
	else ()
		message("Using other Linux hashlib++")
		link_libraries("${CEAGLE_PROJECT_DIR}/lib/linux/libhl++.a")
	endif()
else()
    message("Using macOS hashlib++")
    link_libraries("${CEAGLE_PROJECT_DIR}/lib/macOS/libhl++.a")
endif()

file(GLOB_RECURSE
    CEAGLE_LIB_FILES
    "${CEAGLE_PROJECT_DIR}/src/**/*.cpp"
)

# CMake config files
set(CONFIG_BINARY_NAME ${PROJECT_NAME})
set(CONFIG_VERSION 1.3)
IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
	FIND_PACKAGE(Git)
	IF(GIT_FOUND)
		EXECUTE_PROCESS(
				COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				OUTPUT_VARIABLE "ceagle_BUILD_VERSION"
				ERROR_QUIET
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		MESSAGE( STATUS "Git version: ${ceagle_BUILD_VERSION}" )
	ELSE(GIT_FOUND)
		SET(ceagle_BUILD_VERSION 0)
	ENDIF(GIT_FOUND)
ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
configure_file (
  "${CEAGLE_PROJECT_DIR}/src/Config.h.in"
  "${CEAGLE_PROJECT_DIR}/src/Config.h"
)

# Ceagle executable
add_executable(${CONFIG_BINARY_NAME} ${CEAGLE_PROJECT_DIR}/src/main.cpp
        ${CEAGLE_PROJECT_DIR}/src/Config.h
        )
# link with our own files
add_library(sv-ceagle-lib ${CEAGLE_LIB_FILES})
# link with clang tooling libs
target_link_libraries(sv-ceagle-lib ${llvm_libs})
target_link_libraries(sv-ceagle-lib
	clangTooling
	clangFrontend
	clangSerialization
	clangDriver
	clangParse
	clangSema
	clangAnalysis
	clangAST
	clangASTMatchers
	clangEdit
	clangLex
	clangBasic
)
target_link_libraries(sv-ceagle-lib
	LLVMCore # Support
	LLVMSupport
	LLVMIRReader
	LLVMTransformUtils
	LLVMScalarOpts
	#LLVMX86AsmParser # MC, MCParser, Support, X86Desc, X86Info
	#LLVMX86Desc # MC, Support, X86AsmPrinter, X86Info
	#LLVMX86AsmPrinter # MC, Support, X86Utils
	#LLVMX86Info # MC, Support, Target
	#LLVMX86Utils # Core, Support
	#LLVMipo
	#LLVMInstCombine
	#LLVMipa
	#LLVMAnalysis
	#LLVMTarget
	LLVMOption # Support
	LLVMMCParser # MC, Support
	LLVMMC # Object, Support
	LLVMObject # BitReader, Core, Support
	#LLVMBitReader # Core, Support
)
target_link_libraries(${CONFIG_BINARY_NAME} sv-ceagle-lib)

set_property(TARGET ${CONFIG_BINARY_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET sv-ceagle-lib PROPERTY CXX_STANDARD 14)

#enable_testing()
#add_executable(smt-ast-test ${CEAGLE_TEST_DIR}/SMT/Middleware/test_ast.cpp)
#target_link_libraries(smt-ast-test sv-ceagle-lib)
#set_property(TARGET smt-ast-test PROPERTY CXX_STANDARD 14)
#add_test(NAME smt-ast-test COMMAND smt-ast-test)

#add_executable(smt-solver-test ${CEAGLE_TEST_DIR}/SMT/Middleware/test_solver.cpp)
#target_link_libraries(smt-solver-test sv-ceagle-lib)
#set_property(TARGET smt-solver-test PROPERTY CXX_STANDARD 14)
#add_test(NAME smt-solver-test COMMAND smt-solver-test)

#add_executable(smt-interp-test ${CEAGLE_TEST_DIR}/SMT/Middleware/test_interp.cpp)
#target_link_libraries(smt-interp-test sv-ceagle-lib)
#set_property(TARGET smt-interp-test PROPERTY CXX_STANDARD 14)
#add_test(NAME smt-interp-test COMMAND smt-interp-test)

#add_executable(translator-test ${CEAGLE_TEST_DIR}/SMT/Translator/test_translator.cpp)
#target_link_libraries(translator-test sv-ceagle-lib)
#set_property(TARGET translator-test PROPERTY CXX_STANDARD 14)
#add_test(NAME translator-test COMMAND translator-test)

#add_executable(overflow-test ${CEAGLE_TEST_DIR}/Overflow/test_overflow.cpp)
#target_link_libraries(overflow-test sv-ceagle-lib)
#set_property(TARGET overflow-test PROPERTY CXX_STANDARD 14)
#add_test(NAME overflow-test COMMAND overflow-test)

#add_executable(inst-translator-test ${CEAGLE_TEST_DIR}/SMT/Translator/test_inst_translator.cpp)
#target_link_libraries(inst-translator-test sv-ceagle-lib)
#set_property(TARGET inst-translator-test PROPERTY CXX_STANDARD 14)
#add_test(NAME inst-translator-test COMMAND inst-translator-test)

#add_executable(abstractor-test ${CEAGLE_TEST_DIR}/DFS/Absref/test_abstractor.cpp)
#target_link_libraries(abstractor-test sv-ceagle-lib)
#set_property(TARGET abstractor-test PROPERTY CXX_STANDARD 14)
#add_test(NAME abstractor-test COMMAND abstractor-test)

#add_executable(abstract-state-test ${CEAGLE_TEST_DIR}/DFS/Absref/test_abstract_state.cpp)
#target_link_libraries(abstract-state-test sv-ceagle-lib)
#set_property(TARGET abstract-state-test PROPERTY CXX_STANDARD 14)
#add_test(NAME abstract-state-test COMMAND abstract-state-test)

#add_executable(cegar-test ${CEAGLE_TEST_DIR}/DFS/Absref/test_cegar.cpp)
#target_link_libraries(cegar-test sv-ceagle-lib)
#set_property(TARGET cegar-test PROPERTY CXX_STANDARD 14)
#add_test(NAME cegar-test COMMAND cegar-test)

#add_executable(value-analysis-test ${CEAGLE_TEST_DIR}/ValueAnalysis/test_value_analysis.cpp)
#target_link_libraries(value-analysis-test sv-ceagle-lib)
#set_property(TARGET value-analysis-test PROPERTY CXX_STANDARD 14)
#add_test(NAME value-analysis-test COMMAND value-analysis-test)

#add_executable(absref-arg-manager-test ${CEAGLE_TEST_DIR}/DFS/Advisor/Absref/test_arg_manager.cpp)
#target_link_libraries(absref-arg-manager-test sv-ceagle-lib)
#set_property(TARGET absref-arg-manager-test PROPERTY CXX_STANDARD 14)
#add_test(NAME absref-arg-manager-test COMMAND absref-arg-manager-test)

#add_executable(absref-arg-node-test ${CEAGLE_TEST_DIR}/DFS/Advisor/Absref/test_arg_node.cpp)
#target_link_libraries(absref-arg-node-test sv-ceagle-lib)
#set_property(TARGET absref-arg-node-test PROPERTY CXX_STANDARD 14)
#add_test(NAME absref-arg-node-test COMMAND absref-arg-node-test)

#add_test(NAME ${CONFIG_BINARY_NAME}-test COMMAND ${CONFIG_BINARY_NAME} ../examples/*)

#add_executable(structural-abstraction-test ${CEAGLE_TEST_DIR}/StructuralAbstraction/#test_msg_manager.cpp)
#target_link_libraries(structural-abstraction-test sv-ceagle-lib)
#set_property(TARGET structural-abstraction-test PROPERTY CXX_STANDARD 14)
#add_test(NAME structural-abstraction-test COMMAND structural-abstraction-test)

add_custom_target(
        git-version
        COMMAND ${CMAKE_COMMAND} ..
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_dependencies(${CONFIG_BINARY_NAME} git-version)
