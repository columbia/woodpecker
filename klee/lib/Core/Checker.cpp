#include "klee/Checker.h"
#include "AddressSpace.h"
#include "Executor.h"
#include "Memory.h"
#include "TimingSolver.h"

using namespace klee;
using namespace std;

#include <iostream>

uint64_t Checker::getIntFromExp(ref<Expr> exp) {
    ConstantExpr *ce = dyn_cast<ConstantExpr>(exp);
    if (ce)
    {
        return ce->getZExtValue();
    } else {
        return -1;
    }
}

string Checker::getStrFromCharPtr(ref<Expr> charptr, ExecutionState &state) {
    //            std::cout << funcname << "( " << open_path->getKind() << std::endl;
    bool success = false;
    ObjectPair result;
    state.addressSpace.resolveOne(state, _executor->solver, charptr, result, success);
    ConstantExpr *ce = dyn_cast<ConstantExpr>(charptr);
    if (ce == NULL)
    {
        std::cerr << "warning: char* is not constant!" << std::endl;
    }
    //            std::cout << funcname << "( " << ce->getZExtValue() << std::endl;
    //            Expr *target = (Expr *)ce->getZExtValue();
    //            std::cout << funcname << "( " << target->getKind() << std::endl;

    if (success)
    {
        int offset = 0;
        if (ce)
            offset = ce->getZExtValue() - result.first->address;
        else {
            ref<ConstantExpr> con_charptr;
            success = _executor->solver->getValue(state, charptr, con_charptr);
            if (success) {
                offset = con_charptr->getZExtValue() - result.first->address;
            } else {
                std::cerr << "warning: cannot get offset. default to 0." << std::endl;
            }
        }
        const ObjectState *os = result.second;
        string str = "";
        for (unsigned int i=offset; i<result.first->size; i++)
        {
            ref<Expr> data = os->read(i, 8);
            ConstantExpr *ce = dyn_cast<ConstantExpr>(data);
            if (ce)
            {
                int val = ce->getZExtValue();
                if (val != 0)
                    str += (char)val;
                else
                    break;
            } else {
                std::cerr << "fail to get str: non-const char!" << std::endl;
                return "";
            }
        }
        //                std::cerr << str << "\n";
        return str;
    } else {
        std::cerr << "fail to get str: resolveOne failed!" << std::endl;
        return "";
    }
}

llvm::Function *Checker::getTargetFunctionFromCaller(KInstruction *caller, ExecutionState &state) {
    llvm::CallSite cs(caller->inst);
    llvm::Value *fp = cs.getCalledValue();
    return _executor->getTargetFunction(fp, state);
}

ref<Expr> Checker::getCallingArgument(KInstruction *caller, int number, ThreadState &thr) {
    return _executor->eval(caller, number, thr).value;
}

uint64_t Checker::getPtrFromPPtr(ref<Expr> pptr, ExecutionState &state) {
    if (ConstantExpr *c_pptr = dyn_cast<ConstantExpr>(pptr)) {
        ObjectPair result;
        if (state.addressSpace.resolveOne(c_pptr, result)) {
            ref<Expr> e_ptr = result.second->read(0, result.first->size * 8);
            if (ConstantExpr *c_ptr = dyn_cast<ConstantExpr>(e_ptr)) {
                return c_ptr->getZExtValue();
            }
        }
    }
    std::cerr << "ERROR ERROR getPtrFromPPtr(): fail!" << std::endl;
    return -1;
}
