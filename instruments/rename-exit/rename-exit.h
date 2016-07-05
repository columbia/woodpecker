#ifndef __DIRECTED_SYMBOLIC_EXECUTION_RENAME_EXIT_PASS_H
#define __DIRECTED_SYMBOLIC_EXECUTION_RENAME_EXIT_PASS_H

#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/ADT/DenseSet.h"

#include <vector>

namespace llvm {
  /* Since the exit functions have been made internal (linked with uclibc) in order to handle
  the call edge of atexit(), and LLVM/uclibc does not allow an internal function named exit() and
  will rename it to be exit46() or other names, we have to rename the exit() function before linking
  a bc module with uclibc. */
  struct RenameExitPass: public llvm::ModulePass {
  private:
    
  public:
    static char ID;
    RenameExitPass();
    virtual ~RenameExitPass();
    virtual bool runOnModule(llvm::Module &M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;    
  };
}

#endif

