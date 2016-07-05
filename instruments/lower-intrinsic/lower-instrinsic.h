#ifndef __LOWER_INTRINSIC_PASS_H
#define __LOWER_INTRINSIC_PASS_H

#include "llvm/CodeGen/IntrinsicLowering.h"

namespace llvm {
  struct LowerIntrinsicPass: public ModulePass {
    static char ID;
    llvm::TargetData *TD;
    llvm::IntrinsicLowering *IL;
    bool runOnBasicBlock(llvm::BasicBlock &b);

  public:
    LowerIntrinsicPass()
      : llvm::ModulePass(&ID) {
      TD = NULL;
      IL = NULL;
  	}
    virtual bool runOnModule(llvm::Module &M);
  };
}

#endif
