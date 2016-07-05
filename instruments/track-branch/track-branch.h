#ifndef __DIRECTED_SYMBOLIC_EXECUTION_TRACK_BRANCH_PASS_H
#define __DIRECTED_SYMBOLIC_EXECUTION_TRACK_BRANCH_PASS_H

#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Constant.h"
#include "llvm/ADT/DenseSet.h"

#include <vector>

namespace llvm {
  struct TrackBranchPass: public llvm::ModulePass {
  private:
    const llvm::Type *intType, *voidType;
    const llvm::FunctionType *trackBranchType;
    llvm::Constant *trackBranch;

  protected:
    void declareFunctions(Module &M);
    
  public:
    static char ID;
    TrackBranchPass();
    virtual ~TrackBranchPass();
    virtual bool runOnModule(llvm::Module &M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;    
  };
}

#endif

