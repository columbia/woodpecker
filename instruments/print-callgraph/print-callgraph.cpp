#include "print-callgraph.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "common/callgraph-fp.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

using namespace std;

static RegisterPass<PrintCallGraphPass> X(
		"dsym-print-callgraph",
		"Print call graph.",
		false,
		true); // is analysis

char PrintCallGraphPass::ID = 0;

PrintCallGraphPass::PrintCallGraphPass(): ModulePass(&ID) {
}

PrintCallGraphPass::~PrintCallGraphPass() {}

void PrintCallGraphPass::print(raw_ostream &O, const Module *M) const {

}

void PrintCallGraphPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  //AU.addRequired<CallGraphFP>();
  ModulePass::getAnalysisUsage(AU);
}

bool PrintCallGraphPass::runOnModule(Module &M) {
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
      		for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
            Instruction *instr = i;
            if (instr->getOpcode() == Instruction::Call) {
              Function *calledF = dyn_cast<CallInst>(instr)->getCalledFunction();
		    	if (calledF && !calledF->isDeclaration()) {
		        if (calledFreq.count(calledF) == 0)
		          calledFreq[calledF] = 1;
		        else
		          calledFreq[calledF] = calledFreq[calledF] + 1;
            	}
          	}
          }
      }
	}

  // Print.
  DenseMap<Function *, int>::iterator itr(calledFreq.begin());
  for (; itr != calledFreq.end(); ++itr) {
    errs() << "Callee-freq " << itr->first->getNameStr() << " " << itr->second << "\n";
  }

	// Caller.
	calledFreq.clear();
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
      		for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
            Instruction *instr = i;
            if (instr->getOpcode() == Instruction::Call) {
  		        if (calledFreq.count(f) == 0)
		          calledFreq[f] = 1;
		        else
		          calledFreq[f] = calledFreq[f] + 1;
          	}
          }
      }
	}

  // Print.
  DenseMap<Function *, int>::iterator itr2(calledFreq.begin());
  for (; itr2 != calledFreq.end(); ++itr2) {
    errs() << "Caller-freq " << itr2->first->getNameStr() << " " << itr2->second << "\n";
  }

	// Function pointer.
	calledFreq.clear();
		for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
      		for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
            Instruction *instr = i;
            if (instr->getOpcode() == Instruction::Call) {
              Function *calledF = dyn_cast<CallInst>(instr)->getCalledFunction();
		    	if (!calledF) {
		        if (calledFreq.count(f) == 0)
		          calledFreq[f] = 1;
		        else
		          calledFreq[f] = calledFreq[f] + 1;
            	}
          	}
          }
      }
	}

  // Print.
  DenseMap<Function *, int>::iterator itr3(calledFreq.begin());
  for (; itr3 != calledFreq.end(); ++itr3) {
    errs() << "Call-fp-freq " << itr3->first->getNameStr() << " " << itr3->second << "\n";
  }
  
  return false;
}

