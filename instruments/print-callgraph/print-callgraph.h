#ifndef __DIRECTED_SYMBOLIC_EXECUTION_PRINT_CALL_GRAPH_PASS_H
#define __DIRECTED_SYMBOLIC_EXECUTION_PRINT_CALL_GRAPH_PASS_H

#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/ADT/DenseSet.h"

#include <vector>

namespace llvm {
  struct PrintCallGraphPass: public llvm::ModulePass {
  private:
	llvm::DenseMap<llvm::Function *, int> calledFreq;
    
  public:
    static char ID;
    PrintCallGraphPass();
    virtual ~PrintCallGraphPass();
    virtual bool runOnModule(llvm::Module &M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;    
  };
}

#endif

