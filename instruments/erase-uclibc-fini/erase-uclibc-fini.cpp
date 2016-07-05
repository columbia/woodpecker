#include "erase-uclibc-fini.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
using namespace llvm;

using namespace std;

static RegisterPass<EraseUclibcFiniPass> X(
		"dsym-erase-uclibc-fini",
		"Erase @__uClibc_fini().",
		false,
		true); // is analysis

char EraseUclibcFiniPass::ID = 0;

EraseUclibcFiniPass::EraseUclibcFiniPass(): ModulePass(&ID) {
}

EraseUclibcFiniPass::~EraseUclibcFiniPass() {}

void EraseUclibcFiniPass::print(raw_ostream &O, const Module *M) const {

}

void EraseUclibcFiniPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  ModulePass::getAnalysisUsage(AU);
}

bool EraseUclibcFiniPass::runOnModule(Module &M) {
  Function *f = M.getFunction("tern_exit");
  Function *fini = M.getFunction("__uClibc_fini");
  if(!f || !fini)
    return false;
  for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
    for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {    
      if (i->getOpcode() == Instruction::Call) {
        CallInst *ci = dyn_cast<CallInst>(i);
        assert(ci);
        if (ci->getCalledFunction() == fini) {
          i->eraseFromParent();
          return true;
        }
      }
    }
  }
  return false;
}

