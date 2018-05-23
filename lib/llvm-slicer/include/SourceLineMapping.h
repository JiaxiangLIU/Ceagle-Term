#ifndef _SOURCELINEMAPPING_H
#define _SOURCELINEMAPPING_H

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"

namespace llvm {

// class This pass provides the functionality to find the binary virtual address  and
// binary instruction  corresponding to a llvm instruction.
class SourceLineMappingPass : public ModulePass {
public:
  static char ID;

  SourceLineMappingPass () : ModulePass (ID) {}

  static std::string locateSrcInfo(Instruction *I);

  /// Map all instruction in Module M to source lines
  void mapCompleteFile(Module &M, raw_ostream &Output);

  /// Map all instruction in one function Module M to source lines
  void mapOneFunction(Function *F, raw_ostream &Output);

  /// \brief Using debug information, find the source line number corresponding
  /// to a specified LLVM instruction.
  /// @return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.setPreservesAll();
  };
};

} 

#endif
