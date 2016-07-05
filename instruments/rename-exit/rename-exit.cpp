#include "rename-exit.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
using namespace llvm;

using namespace std;

static RegisterPass<RenameExitPass> X(
		"dsym-rename-exit",
		"Transform exit() to be tern_exit().",
		false,
		true); // is analysis

char RenameExitPass::ID = 0;

RenameExitPass::RenameExitPass(): ModulePass(&ID) {
}

RenameExitPass::~RenameExitPass() {}

void RenameExitPass::print(raw_ostream &O, const Module *M) const {

}

void RenameExitPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  ModulePass::getAnalysisUsage(AU);
}

bool RenameExitPass::runOnModule(Module &M) {
  bool result = false;
  Function *f = M.getFunction("exit");
  if (f) {
    //fprintf(stderr, "Renaming exit to be exit tern_exit...");
    f->setName("tern_exit");
    result = true;
  }
  return result;
}

