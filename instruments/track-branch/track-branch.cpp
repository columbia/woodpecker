#include "track-branch.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/LLVMContext.h"
#include "common/IDAssigner.h"
using namespace llvm;

using namespace std;

static RegisterPass<TrackBranchPass> X(
		"dsym-track-branch",
		"Track outcome of branches",
		false,
		true); // is analysis

char TrackBranchPass::ID = 0;

const static std::string TRACK_BRANCH = "dsym_track_branch";

TrackBranchPass::TrackBranchPass(): ModulePass(&ID) {
}

TrackBranchPass::~TrackBranchPass() {}

void TrackBranchPass::print(raw_ostream &O, const Module *M) const {

}

void TrackBranchPass::getAnalysisUsage(AnalysisUsage &AU) const {
   AU.addRequired<IDAssigner>();
  ModulePass::getAnalysisUsage(AU);
}

bool TrackBranchPass::runOnModule(Module &M) {
  IDAssigner &idm = getAnalysis<IDAssigner>();
  declareFunctions(M);
    for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
      if (b == f->begin())  // Ignore entry basic block of each function.
        continue;
      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {    
        if (i->getOpcode() == Instruction::PHI)
          continue;        
        int instrId = idm.getInstructionID(i);
        assert(instrId != -1);
        CallInst::Create(trackBranch, ConstantInt::get(intType, instrId), "", i);
        break;
      }
    }
  }
  return true;
}

void TrackBranchPass::declareFunctions(Module &M) {
  voidType = Type::getVoidTy(getGlobalContext());
  intType = IntegerType::get(getGlobalContext(), 32);
  trackBranchType = FunctionType::get(voidType, vector<const Type *>(1, intType), false);
  trackBranch = M.getOrInsertFunction(TRACK_BRANCH, trackBranchType);
}


