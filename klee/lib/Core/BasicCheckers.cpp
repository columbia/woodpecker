#include "Common.h"
#include "llvm/Attributes.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Function.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/InstructionInfoTable.h"

#include <fcntl.h>
#include "Memory.h"
#include "TimingSolver.h"
#include "Executor.h"
#include "klee/BasicCheckers.h"
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

using namespace llvm;
using namespace klee;

#define DATALOSS_DEBUG
// #define FORK_CALL_DEBUG

/*int AssertCounter::constBranch = 0;
int AssertCounter::symBranch = 0;
int AssertCounter::assertCount = 0;
int AssertCounter::symAssert = 0;
int AssertCounter::realFork = 0;
int AssertCounter::assertFork = 0;
int AssertCounter::branchCount = 0;*/

std::set<KInstruction *> AssertCounter::asserts;
std::set<KInstruction *> AssertCounter::nonAsserts;

Checker::Result AssertCounter::onBranch(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONBRANCH_ARGUMENTS_WITH_TYPE)
{
//    std::cerr << "=== conditional branch! line:" << ki->info->line << std::endl;
//    std::cerr << ki->info->line << ' ';
    Result retval = OK;

    branchCount++;

    bool isAssert = false;
    if (asserts.find(inst) != asserts.end())
        isAssert = true;
    else if (nonAsserts.find(inst) == nonAsserts.end()) {
        // isAssert defaults to false
        for (unsigned int i=0; i<2; i++)
        {
            BasicBlock *bb = bi->getSuccessor(i);
            BasicBlock::iterator iter = bb->begin();
            while (iter)
            {
                Instruction *inst = iter;
                if (CallInst *ci = dyn_cast<CallInst>(inst))
                {
                    const Value* value = ci->getCalledValue();
                    std::string callee = value->getName();
                    if (callee == "__assert_fail")
                    {
                        isAssert = true;
                        break;
                    }
                    if ((callee != "llvm.debug.value") && (callee != "llvm.debug.declare"))
                        break;
                } else break;

                iter++;
            }

            if (isAssert)
                break;
        }

        if (isAssert)
            asserts.insert(inst);
        else
            nonAsserts.insert(inst);
    }
    
    bool isConst = true;

    if (isa<ConstantExpr>(cond))
    {
        constBranch++;
//        std::cerr << "constant expr!" << std::endl;
    }
    else
    {
        isConst = false;
        symBranch++;
//        std::cerr << "non-const expr!" << std::endl;
    }


    if (isAssert)
    {
        assertCount++;
//        std::cerr << "assert!" << std::endl;
        if (!isConst)
        {
            symAssert++;
        }
        retval = IMPORTANT;
    }
    else
    {
//        std::cerr << "not assert!" << std::endl;
//    if (assert_lines.find(ki->info->line) != assert_lines.end())
    }

    if (trueBranch && falseBranch)
    {
        realFork++;

        if (isAssert)
        {
            assertFork++;
//        if (assert_lines.find(ki->info->line) != assert_lines.end())
//            std::cerr << "symbolic assert fork!" << std::endl;
        }
//        std::cerr << "real fork!" << std::endl;
    }
//    std::cerr << "===" << std::endl;
    return retval;
 
}

bool AssertCounter::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "__assert_fail")
        return true;
    return false;
}

AssertCounter::AssertCounter(Executor *executor)
    : Checker(executor),
constBranch(0),
symBranch(0),
assertCount(0),
symAssert(0),
realFork(0),
assertFork(0),
branchCount(0)
{
}

Checker::Result AssertCounter::report(ExecutionState *state)
{
    ofstream outf("assert_info.txt");

    outf << "Branch: " << branchCount << std::endl;
    outf << "Symbolic Branch: " << symBranch << std::endl;
    outf << "Normal Branch: " << constBranch << std::endl;
    outf << "Asserts: " << assertCount << std::endl;
    outf << "Symbolic Asserts: " << symAssert << std::endl;
    outf << "Forks: " << realFork << std::endl;
    outf << "Assert Forks: " << assertFork << std::endl;

    outf.flush();

    return OK;
}

std::set<KInstruction *> AssertChecker::asserts;
std::set<KInstruction *> AssertChecker::nonAsserts;

Checker::Result AssertChecker::onBranch(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONBRANCH_ARGUMENTS_WITH_TYPE)
{
//    std::cerr << "=== conditional branch! line:" << inst->info->line << std::endl;
    Result retval = OK;

    bool isAssert = false;
    if (asserts.find(inst) != asserts.end())
    {
//        std::cerr << "known Assert";
        isAssert = true;
    }
    else if (nonAsserts.find(inst) == nonAsserts.end()) {
        // isAssert defaults to false
        for (int i=0; i<2; i++)
        {
            BasicBlock *bb = bi->getSuccessor(i);
            BasicBlock::iterator iter = bb->begin();
            while (iter)
            {
                Instruction *inst = iter;
                if (CallInst *ci = dyn_cast<CallInst>(inst))
                {
                    const Value* value = ci->getCalledValue();
                    std::string callee = value->getName();
                    if (callee == "__assert_fail")
                    {
                        isAssert = true;
                        break;
                    }
                    if ((callee != "llvm.debug.value") && (callee != "llvm.debug.declare"))
                        break;
                } else {
//                    std::cerr << "not call: not assert" << std::endl;
//                    std::cerr << inst->getOpcodeName() << std::endl;
                    break;
                }

                iter++;
            }

            if (isAssert)
                break;
        }

        if (isAssert)
            asserts.insert(inst);
        else
            nonAsserts.insert(inst);
    } else {
//        std::cerr << "known not Assert" << std::endl;
    }
    
    bool isConst = isa<ConstantExpr>(cond);

    if (isAssert)
    {
//        std::cerr << "assert!" << std::endl;
        if (!isConst)
        {
            retval = IMPORTANT;
//            std::cerr << "symbolic assert!" << std::endl;
        }
    }

    return retval;
 
}

bool AssertChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "__assert_fail")
        return true;
    return false;
}

AssertChecker::AssertChecker(Executor *executor)
    : Checker(executor)
{
}

Checker::Result AssertChecker::report(ExecutionState *state)
{
    return OK;
}

bool OpenCloseChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "fopen" || funcname == "fdopen" || funcname == "fopen64" || funcname == "fclose")
        return true;
    return false;
}

bool OpenCloseChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}

Checker::Result OpenCloseChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE
        )
{
    Result retval = OK;
    string funcname = f->getNameStr();
    uint64_t need_args = 0;

    if ((funcname == "fopen" || funcname == "fopen64" || funcname == "fdopen") && args != NULL)
    {
        uint64_t fd = args[0];
//        cout << "concrete open " << hex << fd << ' ' << (long)target->inst << endl;
        if (fd != 0) {
            open_fds.insert(fd);
            AllocInfo alloc_info(target);
            alloc_info.addThread(ThreadInfo(thr));
            open_fds_allocinfo.insert(std::pair<uint64_t, AllocInfo>(fd, alloc_info));
        }
        need_args = NEED_RET;
        retval = IMPORTANT | need_args;
    } else if (funcname == "fclose") {
        need_args = NEED_ARG0;
        if (args != NULL)
        {
            uint64_t fd = args[2];
//            cout << "close " << hex << fd << ' ' << (long)arguments[0].get() << endl;
            if (!open_fds.count(fd))
            {
                std::cerr << "Error! Closed not opened!" << std::endl;
                return ERROR | need_args;
            }
            open_fds.erase(fd);
            open_fds_allocinfo.erase(fd);
        } else {
            ref<Expr> close_fd = arguments[0];
            std::set<ref<Expr> >::iterator match_fd;
            bool found = false;
            for (std::set<ref<Expr> >::iterator it = open_fds_exp.begin();
                    it != open_fds_exp.end(); it++)
            {
                ref<Expr> match = EqExpr::create(close_fd, *it);
                bool result;
                bool success = _executor->solver->mustBeTrue(state, match, result);
                if (!success)
                {
                    std::cerr << "Error! Solver error matching FDs!" << std::endl;
                    return ERROR | need_args;
                }
                if (result)
                {
                    found = true;
                    match_fd = it;
                    break;
                }
            }
            if (!found)
            {
                if (dismiss_error)
                    return OK;
                if (isa<ConstantExpr>(close_fd))
                {
//                    std::cerr << "is const!" << std::endl;
                    ConstantExpr *ce = dyn_cast<ConstantExpr>(close_fd);
//                    std::cerr << std::hex << ce->getZExtValue() << std::endl;
                    ObjectPair result;
                    if (state.addressSpace.resolveOne(ce, result))
                    {
 //                       std::cerr << result.first->name << "\n";
                        if (result.first->name == "_stdio_streams")
                        {
                            return OK;
                        }
                    }
                }
                std::cerr << "Error! CLOSE without OPEN!" << std::endl;
                return ERROR | need_args;
            }
            open_fds_exp.erase(match_fd);
            open_fds_exp_allocinfo.erase(match_fd->get());
        }
        retval = IMPORTANT | need_args;
    } else if (funcname == "close_stdout" && args == NULL)
    {
        dismiss_error = true;
    }

    return retval;
}

Checker::Result OpenCloseChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Result retval = OK;
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return retval;
    string funcname = f->getName();
    uint64_t need_args;

    if (funcname == "fopen" || funcname == "fopen64" || funcname == "fdopen")
    {
        need_args = NEED_RET;
        ConstantExpr *ce;
        if ((ce = dyn_cast<ConstantExpr>(result)) != NULL)
        {
            if (ce->getZExtValue() != 0)
            {
                open_fds_exp.insert(result);
                AllocInfo alloc_info(caller);
                alloc_info.addThread(ThreadInfo(thr));
                open_fds_exp_allocinfo.insert(std::pair<Expr*, AllocInfo>(result.get(), alloc_info));
            }
        }
        retval = IMPORTANT | need_args;
    } else if (funcname == "close_stdout") {
        dismiss_error = false;
    }

    return retval;
}

void AllocInfo::gatherPlaceInfo(ExecutionState *state, std::string &place_info, uint64_t &place_hash) {
    if (state != NULL) {
        ThreadState *thr = state->lastThread();
        if (thr != NULL) {
            place_info = ThreadInfo(*thr).callStack(false);
        }
    }

    place_hash = AllocInfo::hash(place_info.c_str());
}

bool AllocInfo::isReported(multimap<uint64_t, std::string>& record, const string& place_info, uint64_t place_hash, const string& case_info) {
    uint64_t case_hash = place_hash + AllocInfo::hash(case_info.c_str());
    bool isreported = false;
    string global_info = place_info + case_info;
    for (multimap<uint64_t, std::string>::iterator rit = record.equal_range(case_hash).first;
            rit != record.equal_range(case_hash).second; rit++) {
        if ((*rit).second == global_info) {
            isreported = true;
            break;
        }
    }
    if (isreported) return true;

    record.insert(pair<uint64_t, string>(case_hash, global_info));
    return false;
}

std::multimap<uint64_t, std::string> OpenCloseChecker::reported;

Checker::Result OpenCloseChecker::report(ExecutionState *state)
{
    string place_info;
    uint64_t place_hash;
    AllocInfo::gatherPlaceInfo(state, place_info, place_hash);
//    cout << "=== PATH END ===" << endl;
    bool error = false;
    if (!open_fds.empty())
    {
        cerr << "concrete fd not closed! count: " << open_fds.size() << endl;
        for (set<uint64_t>::iterator it = open_fds.begin();
                it != open_fds.end(); it++)
        {
            string case_info = open_fds_allocinfo.find(*it)->second.toString(false);
            if (AllocInfo::isReported(reported, place_info, place_hash, case_info)) {
                std::cerr << "ERROR SUPPRESSED: reported\n";
                continue;
            }

            error = true;
            cerr << "not closed: " << hex << *it << endl;
            cerr << "\t\topened: " << open_fds_allocinfo.find(*it)->second.toString() << std::endl;
        }
    }
    if (!open_fds_exp.empty())
    {
        cerr << "symbolic fd not closed! count: " << open_fds_exp.size() << endl;
        for (set<ref<Expr> >::iterator it = open_fds_exp.begin();
                it != open_fds_exp.end(); it++)
        {
            string case_info = open_fds_exp_allocinfo.find(it->get())->second.toString(false);
            if (AllocInfo::isReported(reported, place_info, place_hash, case_info)) {
                std::cerr << "ERROR SUPPRESSED: reported\n";
                continue;
            }

            error = true;
            cerr << "not closed: " << hex << *it << endl;
            cerr << "\t\topened: " << open_fds_exp_allocinfo.find(it->get())->second.toString() << std::endl;
        }
    }
    if (error) {
        std::cerr << "Exit path: " << std::endl;
        std::cerr << place_info << std::endl;
        return ERROR;
    } else return OK;
}

bool LockChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "pthread_mutex_init" ||
        (funcname == "pthread_mutex_lock") ||
        (funcname == "pthread_mutex_unlock") ||
        (funcname == "pthread_mutex_destroy"))
    {
        return true;
    } else {
        return false;
    }

}

bool LockChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    string funcname = f->getNameStr();
    if (funcname == "pthread_mutex_init" ||
        (funcname == "pthread_mutex_lock") ||
        (funcname == "pthread_mutex_unlock") ||
        (funcname == "pthread_mutex_destroy"))
    {
        return true;
    } else {
        return false;
    }
}

Checker::Result LockChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE)
{
    Result retval = OK;
    string funcname = f->getNameStr();
    uint64_t need_args = 0;

    if (args != NULL)
    {
        if (funcname == "pthread_mutex_init")
        {
            need_args = NEED_ARG0;
            uint64_t mutex = args[2];
            if (init_locks.find(mutex) != init_locks.end())
            {
                cerr << "Lock already inited! " << hex << mutex << endl;
                return ERROR | need_args;
            }
            init_locks.insert(mutex);
            retval = IMPORTANT | need_args;
        } else if (funcname == "pthread_mutex_lock")
        {
            need_args = NEED_ARG0;
            uint64_t mutex = args[2];
            if (init_locks.find(mutex) == init_locks.end())
            {
                cerr << "Lock not inited! " << hex << mutex << endl;
                return ERROR | need_args;
            }
            if (locked_locks.find(mutex) != locked_locks.end())
            {
                cerr << "Lock already locked" << endl;
                return ERROR | need_args;
            }
            locked_locks.insert(mutex);
            retval = IMPORTANT | need_args;
        } else if (funcname == "pthread_mutex_unlock")
        {
            need_args = NEED_ARG0;
            uint64_t mutex = args[2];
            if (init_locks.find(mutex) == init_locks.end())
            {
                cerr << "Lock not inited! " << hex << mutex << endl;
                return ERROR | need_args;
            }
            if (locked_locks.find(mutex) == locked_locks.end())
            {
                cerr << "Unlock but not locked" << endl;
                return ERROR | need_args;
            }
            locked_locks.erase(mutex);
            retval = IMPORTANT | need_args;
        } else if (funcname == "pthread_mutex_destroy")
        {
            need_args = NEED_ARG0;
            uint64_t mutex = args[2];
            if (locked_locks.find(mutex) != locked_locks.end())
            {
                cerr << "Destroying a locked lock!" << endl;
                return ERROR | need_args;
            }
            if (init_locks.find(mutex) == init_locks.end())
            {
                cerr << "Destroying an uninited lock!" << endl;
                return ERROR | need_args;
            }
            init_locks.erase(mutex);
            retval = IMPORTANT | need_args;
        }

    }
    return retval;
}

Checker::Result LockChecker::report(ExecutionState *state)
{
    bool error = false;
    if (!init_locks.empty())
    {
        cerr << "locks not destroyed! count: " << init_locks.size() << endl;
        error = true;
        for (std::set<uint64_t>::iterator it = init_locks.begin();
                it != init_locks.end(); it++)
        {
            cerr << "not destroyed: " << hex << *it << endl;
        }
    }
    if (!locked_locks.empty())
    {
        cerr << "locks not unlocked! count: " << locked_locks.size() << endl;
        error = true;
        for (std::set<uint64_t>::iterator it = locked_locks.begin();
                it != locked_locks.end(); it++)
        {
            cerr << "not unlocked: " << hex << *it << endl;
        }
    }
    if (error)
        return ERROR;
    return OK;
}

bool FileChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "clearerr" ||
        funcname == "clearerr_unlocked")
        return true;
    if (funcname.length() < 5)
        return false;
    if (funcname[0] != 'f')
        return false;
    if (funcname == "fopen" ||
        funcname == "fopen64" ||
        funcname == "fdopen" ||
        funcname == "fclose" ||
        funcname == "fputs" ||
        funcname == "fputs_unlocked" ||
        funcname == "fgets" ||
        funcname == "fgets_unlocked" ||
        funcname == "fprintf" ||
        funcname == "fread" ||
        funcname == "fread_unlocked" ||
        funcname == "fwrite" ||
        funcname == "fwrite_unlocked" ||
        funcname == "ferror" ||
        funcname == "ferror_unlocked"
        )
        return true;
    return false;
}

bool FileChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}

Checker::Result FileChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE)
{
    uint64_t need_args = 0;
    string funcname = f->getNameStr();
    if (funcname == "clearerr" || funcname == "clearerr_unlocked") {
        need_args = NEED_ARG0;
        uint64_t fp;
        if (args != NULL) {
            fp = args[2];
        } else {
            fp = getIntFromExp(arguments[0]);
        }

        if (err_fds.count(fp)) {
            err_fds.erase(fp);
            open_fds.insert(fp);
            // although you cleared it... can you read it?....
        }

        if (err_dirty_fds.count(fp)) {
            err_dirty_fds.erase(fp);
            dirty_fds.insert(fp);
            // dirty and error... what a poor fp...
        }
        return IMPORTANT | need_args;
    }

    if (funcname.length() < 5 || funcname[0] != 'f')
        return OK;

    std::cerr << funcname << std::endl;

    if (funcname == "fopen" || funcname == "fopen64" || funcname == "fdopen")
    {
        need_args = NEED_RET;
        if (args != NULL)
        {
            uint64_t fd = args[0];
            if (fd != 0)
            {
#ifdef FILECHECKER_DEBUG
                std::cerr << funcname << "() : " << fd << "\n";
#endif
                open_fds.insert(fd);
            }
            return IMPORTANT | need_args;
        } // else: handled in onRet
    } else if (funcname == "fclose")
    {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args != NULL)
        {
            fd = args[2];
        } else {
            ref<Expr> close_fd = arguments[0];
            if (isa<ConstantExpr>(close_fd))
            {
                ConstantExpr *ce = dyn_cast<ConstantExpr>(close_fd);
                ObjectPair result;
                if (state.addressSpace.resolveOne(ce, result))
                {
                    if (result.first->name == "_stdio_streams")
                    {
                        return OK;
                    }
                }
            }

            fd = getIntFromExp(close_fd);
        }

#ifdef FILECHECKER_DEBUG
        std::cerr << funcname << "(" << fd << ")\n";
#endif
        bool found = false;
        if (open_fds.count(fd))
        {
            open_fds.erase(fd);
            found = true;
        }
        if (err_fds.count(fd))
        {
            err_fds.erase(fd);
            found = true;
        }
        if (dirty_fds.count(fd))
        {
            dirty_fds.erase(fd);
            found = true;
        }
        if (err_dirty_fds.count(fd))
        {
            err_dirty_fds.erase(fd);
            found = true;
        }
        if (!found)
        {
            std::cerr << "Error! Closed not opened!" << std::endl;
            return ERROR | need_args;
        }

        return IMPORTANT | need_args;
    } else if (funcname == "ferror" || funcname == "ferror_unlocked") {
        need_args = NEED_RET | NEED_ARG0;
        if (args != NULL) {
            uint64_t ret = args[0];
            uint64_t fp = args[2];
            if (ret != 0) {
                if (open_fds.count(fp)) {
                    open_fds.erase(fp);
                    err_fds.insert(fp);
                } else if (err_fds.count(fp) || err_dirty_fds.count(fp)) {
                    // tested already...
                } else if (dirty_fds.count(fp)){
                    dirty_fds.erase(fp);
                    err_dirty_fds.insert(fp);
                } else {
                    // fd not opened?....
                }
            }
        } // else: handled in onRet()
        return IMPORTANT | need_args;
    } else {
        uint64_t fp;
        int fd_idx = -2;
        if (funcname == "fread" || funcname == "fwrite" ||
            funcname == "fread_unlocked" || funcname == "fwrite_unlocked") {
            fd_idx = 3;
        } else if (funcname == "fputs" || funcname == "fputs_unlocked") {
            fd_idx = 1;
        } else if (funcname == "fgets" || funcname == "fgets_unlocked") {
            fd_idx = 2;
        } else if (funcname == "fprintf") {
            fd_idx = 0;
        } // -1: ret val (maybe useful)
        if (fd_idx < -1)
        {
            return OK;
        }
        if (fd_idx < 0) {
            need_args = NEED_RET;
        } else {
            need_args = 1 << (fd_idx + NEED_ARG_BEGIN);
        }
        if (args == NULL)
        {
            // symbolic check
            ref<Expr> op_fd = arguments[fd_idx];
            // ignore std*
            if (isa<ConstantExpr>(op_fd))
            {
                ConstantExpr *ce = dyn_cast<ConstantExpr>(op_fd);
                ObjectPair result;
                if (state.addressSpace.resolveOne(ce, result))
                {
                    if (result.first->name == "_stdio_streams")
                    {
                        return OK;
                    }
/*
struct __STDIO_FILE_STRUCT {
	unsigned short __modeflags;
	// There could be a hole here, but modeflags is used most.
#ifdef __UCLIBC_HAS_WCHAR__
	unsigned char __ungot_width[2]; // 0: current (building) char; 1: scanf 
	// Move the following futher down to avoid problems with getc/putc
	// macros breaking shared apps when wchar config support is changed. 
	// wchar_t ungot[2];
#else  // __UCLIBC_HAS_WCHAR__
	unsigned char __ungot[2];
#endif // __UCLIBC_HAS_WCHAR__
	int __filedes;
};
*/
                    ref<Expr> fd_exp = result.second->read(4, 32);
                    if (isa<ConstantExpr>(fd_exp))
                    {
                        int fd = dyn_cast<ConstantExpr>(fd_exp)->getZExtValue();
                        if (fd < 0)
                        {
//                            std::cerr << "fake, ignore" << std::endl;
                            // fake file stream
                            return OK;
                        } else {
//                            std::cerr << "file des: " << fd << std::endl;
                        }
                    }

                }
            }

            fp = getIntFromExp(op_fd);
        } else {
//            for (int i=0; i<10; i++)
//                std::cerr << "arg " << i << ": " << hex << args[i] << std::endl;
            fp = args[2 + fd_idx]; // U128 retv, U64 arg0,arg1 ...
            FILE *rfp = (FILE *)fp;
            if (rfp->_fileno < 0)
                return OK;
            if (rfp->_fileno <= 2 && rfp->_fileno >= 0)
                return OK;
        }
        if (!open_fds.count(fp))
        {
            if (err_fds.count(fp) || err_dirty_fds.count(fp))
            {
                std::cerr << "Error! " << funcname << "() on a fp with ferror()!\n";
                return ERROR | need_args;
            } else if (dirty_fds.count(fp)) {
                // dirty... still dirty
            } else {
                std::cerr << "Error! " << funcname << "() without OPEN!" << std::endl;
                return ERROR | need_args;
            }
        } else {
            open_fds.erase(fp);
            dirty_fds.insert(fp);
        }
        return IMPORTANT | need_args;
    }

    return OK;
}
 
Checker::Result FileChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return OK;
    string funcname = f->getName();
    uint64_t need_args = 0;

    if (funcname == "fopen" || funcname == "fopen64" || funcname == "fdopen")
    {
        uint64_t fp = getIntFromExp(result);
        if (fp != 0)
            open_fds.insert(fp);
        else
            std::cerr << "warning: " << funcname << "() failed.\n";
        need_args = NEED_RET;
        return IMPORTANT | need_args;
    } else if (funcname == "ferror" || funcname == "ferror_unlocked") {
        uint64_t ret = getIntFromExp(result);
        uint64_t fp = getIntFromExp(getCallingArgument(caller, 1, thr));
        need_args = NEED_RET | NEED_ARG0;
        if (ret != 0) { // error
            if (open_fds.count(fp)) {
                open_fds.erase(fp);
                err_fds.insert(fp);
            } else if (err_fds.count(fp) || err_dirty_fds.count(fp)) {
                // tested already...
            } else if (dirty_fds.count(fp)) {
                dirty_fds.erase(fp);
                err_dirty_fds.insert(fp);
            } else {
                // fd not opened?....
            }
        }
        return IMPORTANT | need_args;
    }

    return OK;
}

Checker::Result FileChecker::report(ExecutionState *state)
{
//    cout << "=== PATH END ===" << endl;
    bool error = false;
    if (!dirty_fds.empty())
    {
        cerr << "concrete dirty FILE* not closed! count: " << dirty_fds.size() << endl;
        for (set<uint64_t>::iterator it = dirty_fds.begin();
                it != dirty_fds.end(); it++)
        {
            cerr << "not closed: " << hex << *it << endl;
        }
        error = true;
    }
    if (!err_dirty_fds.empty())
    {
        cerr << "concrete dirty FILE* with error not closed! count: " << err_dirty_fds.size() << endl;
        for (set<uint64_t>::iterator it = err_dirty_fds.begin();
                it != err_dirty_fds.end(); it++)
        {
            cerr << "not closed: " << hex << *it << endl;
        }
        error = true;
    }

    if (error)
        return ERROR;

//    cout << "OK" << endl;
    return OK;
}

bool DataLossChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "fopen" ||
            funcname == "fopen64" ||
            funcname == "fdopen" ||
            funcname == "open" ||
            funcname == "open64" ||
            funcname == "fputs" ||
            funcname == "fputs_unlocked" ||
            funcname == "fwrite" ||
            funcname == "fwrite_unlocked" ||
            funcname == "fprintf" ||
            funcname == "fflush" ||
            funcname == "fflush_unlocked" ||
            funcname == "fsync" ||
            funcname == "fdatasync" ||
            funcname == "fileno" ||
            funcname == "fileno_unlocked" ||
            funcname == "fclose" ||
            funcname == "unlink" ||
            funcname == "creat" ||
            funcname == "rename")
        return true;
    return false;
}

bool DataLossChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE) {
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}

Checker::Result DataLossChecker::parseFOpen(const std::string &filename,
        const std::string &mode, uint64_t fp) {
    if (fp == 0)
    {
        // fopen failed!
#ifdef DATALOSS_DEBUG
        std::cerr << "warning: fopen() failed.\n";
#endif
        return IMPORTANT;
    }

    if (filename == "" || mode == "") {
        // failed!
        return IMPORTANT;
    }

    if (mode.length() > 0)
        if (mode[0] != 'r') // r: will not create file
            if (unlinked_files.count(filename)) {
                unlinked_files.erase(filename);
            }

    FileObject file(filename, fp);
    file.setState(FileObject::CLEAN);
    files.push_back(file);
    return IMPORTANT;
}

Checker::Result DataLossChecker::parseOpen(const std::string &filename,
        uint64_t flag, uint64_t fd) {
    if (fd == (uint32_t)-1)
    {
        // open failed!
#ifdef DATALOSS_DEBUG
        std::cerr << "warning: open() failed.\n";
#endif
        return IMPORTANT;
    }

    if (filename == "") {
        // failed!
        return IMPORTANT;
    }

//    std::cerr << fd << std::endl;

    if (flag & O_CREAT) {
        if (unlinked_files.count(filename)) {
            unlinked_files.erase(filename);
        }
    }

    FileObject file(filename, -1, fd);
    file.setState(FileObject::CLEAN);
    files.push_back(file);
    return IMPORTANT;
}

Checker::Result DataLossChecker::parseFdOpen(uint64_t fd, const std::string &mode, uint64_t fp) {
    if (fp == 0) {
#ifdef DATALOSS_DEBUG
        std::cerr << "warning: fdopen() failed.\n";
#endif
        return IMPORTANT;
    }

    bool found = false;
    FileObject *frec = NULL;
    for (unsigned i = 0; i<files.size(); i++) {
        if (files[i].getFd() == fd) {
            found = true;
            frec = &files[i];
            if (files[i].getFp() == (uint64_t)-1) {
                files[i].setFp(fp);
                return IMPORTANT;
            }
        }
    }

    if (found) {
        // maybe already fdopen()ed
        // or maybe fopen()->fileno()->fdopen()
        FileObject file(frec->getName(), fp, fd);
        file.setState(FileObject::CLEAN);
        files.push_back(file);
        return IMPORTANT;
    }

    // not found? we did not capture open()?
#ifdef DATALOSS_DEBUG
    std::cerr << "warning: fdopen(): can't find corresponding fd\n" << std::endl;
#endif

    // maybe incorrect...
    FileObject file("", fp, fd);
    file.setState(FileObject::CLEAN);
    files.push_back(file);
    return IMPORTANT;
}

Checker::Result DataLossChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE
        )
{
    string funcname = f->getNameStr();
    uint64_t need_args = 0;
    if (funcname == "fopen" || funcname == "fopen64")
    {
        need_args = NEED_ARG0 | NEED_ARG1 | NEED_RET;
        if (args != NULL)
        {
            string filename = string((char*)args[2]);
            string mode = string((char*)args[3]);
//            std::cout << funcname << "( " << filename << std::endl;
            uint64_t fp = args[0];
#ifdef DATALOSS_DEBUG
            std::cerr << funcname << "(" << filename << "," << mode << ") = " << fp << std::endl;
#endif
            return parseFOpen(filename, mode, fp) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "open" || funcname == "open64")
    {
        need_args = NEED_ARG0 | NEED_ARG1 | NEED_RET;
        if (args != NULL)
        {
            string filename = string((char*)args[2]);
            uint64_t flag = args[3];
            uint64_t fd = args[0];
#ifdef DATALOSS_DEBUG
            std::cerr << funcname << "(" << filename << "," << flag << ") = " << fd << std::endl;
#endif
            return parseOpen(filename, flag, fd) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "creat")
    {
        if (args != NULL)
        {
            need_args = NEED_RET | NEED_ARG0;
            string filename = string((char*)args[2]);
            uint64_t fd = args[0];
            uint64_t flag = O_WRONLY | O_CREAT | O_TRUNC;
#ifdef DATALOSS_DEBUG
            std::cerr << funcname << "(" << filename << "," << flag << ") = " << fd << std::endl;
#endif
            return parseOpen(filename, flag, fd) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "fputs" ||
            funcname == "fputs_unlocked" ||
            funcname == "fprintf" ||
            funcname == "fwrite" ||
            funcname == "fwrite_unlocked") {
        int fp_idx = -2;
        if (funcname == "fwrite" || funcname == "fwrite_unlocked")
        {
            fp_idx = 3;
        } else if (funcname == "fputs" || funcname == "fputs_unlocked")
        {
            fp_idx = 1;
        } else if (funcname == "fprintf") {
            fp_idx = 0;
        }
        uint64_t fp;
        if (args == NULL)
        {
            fp = getIntFromExp(arguments[fp_idx]);
        } else {
            fp = args[2+fp_idx];
        }
        need_args = 1 << (fp_idx + NEED_ARG_BEGIN);
#ifdef DATALOSS_DEBUG
            std::cerr << funcname << "(" << fp << ",...)" << std::endl;
#endif
        for (unsigned int i=0; i<files.size(); i++) {
            if (files[i].getFp() == fp) {
                files[i].setState(FileObject::LIB_BUFFERED);
                break;
            }
        }
        return IMPORTANT | need_args;
    } else if (funcname == "fdopen") {
        need_args = NEED_ARG0 | NEED_ARG1 | NEED_RET;
        if (args) {
            uint64_t fp = args[0];
            uint64_t fd = args[2];
            string mode = string((char*)args[3]);
#ifdef DATALOSS_DEBUG
            std::cerr << funcname << "(" << fd << "," << mode << ") = " << fp << std::endl;
#endif
            return parseFdOpen(fd, mode, fp) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "write") {
        uint64_t fd;
        int write_len;
        if (args == NULL)
        {
            fd = getIntFromExp(arguments[0]);
            write_len = getIntFromExp(arguments[2]);
        } else {
            fd = args[2];
            write_len = args[4];
        }
        if (fd <= 2) {
            // don't care about stdout/stderr
            return OK;
        }
        need_args = NEED_ARG0;
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << fd << ",..," << write_len << ")" << std::endl;
#endif
        for (unsigned int i=0; i<files.size(); i++) {
            if (files[i].getFd() == fd) {
#ifdef DATALOSS_DEBUG
                std::cerr << "found entry, state " << files[i].getState() << std::endl;
#endif
                if (files[i].getState() == FileObject::CLEAN) {
                    files[i].setState(FileObject::OS_BUFFERED);
                }
                break;
            }
        }
        return IMPORTANT | need_args;
    } else if (funcname == "fflush" || funcname == "fflush_unlocked"
            || funcname == "fclose") {
        uint64_t fp;
        if (args != NULL) {
            fp = args[2];
        } else {
            fp = getIntFromExp(arguments[0]);
        }
        need_args = NEED_ARG0;
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << fp << ")" << std::endl;
#endif
        for (unsigned int i=0; i<files.size(); i++)
            if (files[i].getFp() == fp) {
                FileObject::FileState state = files[i].getState();
#ifdef DATALOSS_DEBUG
                std::cerr << "found file." << std::endl;
#endif
                if (state == FileObject::LIB_BUFFERED) {
                    files[i].setState(FileObject::OS_BUFFERED);
                }
                if (funcname == "fclose")
                {
                    files[i].setFp(-1); // destroyed!
                    files[i].setFd(-1); // destroyed!
                }
                break;
            }
        return IMPORTANT | need_args;
    } else if (funcname == "close") {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args != NULL) {
            fd = args[2];
        } else {
            fd = getIntFromExp(arguments[0]);
        }
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << fd << ")" << std::endl;
#endif

        for (unsigned int i=0; i<files.size(); i++)
        {
            if (files[i].getFd() == fd)
            {
                if (files[i].getFp() != (uint64_t)-1)
                {
                    std::cerr << "closing internal fd!\n";
                    return ERROR | need_args;
                }
                files[i].setFd(-1);
            }
        }
        return IMPORTANT | need_args;
    } else if (funcname == "fileno" || funcname == "fileno_unlocked") {
        need_args = NEED_ARG0 | NEED_RET;
        if (args != NULL) {
            uint64_t fp = args[2];
            uint64_t fd = args[0];
            for (unsigned int i=0; i<files.size(); i++)
                if (files[i].getFp() == fp)
                {
                    files[i].setFd(-1);
                    for (unsigned int j=0; j<files.size(); j++) {
                        if (files[j].getFd() == fd) {
                            // corresponding entry, remove it
                            files.erase(files.begin() + j);
                            j--;
                        }
                    }
                    files[i].setFd(fd);
                    break;
                }
        }
        return IMPORTANT | need_args;
    } else if (funcname == "fsync" || funcname == "fdatasync") {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args != NULL) {
            fd = args[2];
        } else {
            fd = getIntFromExp(arguments[0]);
        }
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(): " << fd << std::endl;
#endif
        for (unsigned int i=0; i<files.size(); i++)
            if (files[i].getFd() == fd)
            {
                FileObject::FileState state = files[i].getState();
#ifdef DATALOSS_DEBUG
                std::cerr << "found corresponding file." << std::endl;
#endif
                if (state == FileObject::OS_BUFFERED)
                {
                    files[i].setState(FileObject::CLEAN);
                }
#ifdef DATALOSS_DEBUG
                else 
                    std::cerr << "state: " << state << std::endl;
#endif
                break;
            }
        return IMPORTANT | need_args;
    } else if (funcname == "unlink") {
        need_args = NEED_ARG0;
        string filename;
        if (args == NULL) {
            filename = getStrFromCharPtr(arguments[0], state);
        } else {
            filename = string((char*)args[2]);
        }
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << filename << ",...)\n";
#endif
        if (filename == "") {
            return IMPORTANT | need_args;
        }

        unlinked_files.insert(filename);
        return IMPORTANT | need_args;

    } else if (funcname == "rename") {
        need_args = NEED_ARG0 | NEED_ARG1;
        string filename;
        string target;
        if (args == NULL) {
            filename = getStrFromCharPtr(arguments[0], state);
            target = getStrFromCharPtr(arguments[1], state);
        } else {
            filename = string((char*)args[2]);
            target = string((char*)args[3]);
        }
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << filename << " -> " << target << ")\n";
#endif
        if (filename == "" || target == "") {
            // failed!
            return IMPORTANT | need_args;
        }
        if (unlinked_files.count(target)) {
            std::cerr << "ERROR! rename() to an unlink()ed file! Possible data loss!\n" << std::endl;
            return ERROR | need_args;
        }
        if (filename != target)
            // not really unlinked, but renamed to another name
            unlinked_files.insert(filename);
        for (unsigned int i=0; i<files.size(); i++)
            // XXX: should also consider equivalent paths
            if (files[i].getName() == filename)
            {
                FileObject::FileState state = files[i].getState();
#ifdef DATALOSS_DEBUG
                std::cerr << "found matched entry" << std::endl;
#endif
                if (state == FileObject::LIB_BUFFERED)
                {
                    std::cerr << "ERROR! rename() without fflush()! file: " << filename << " fd: " << (int)files[i].getFd() << std::endl;
                    return ERROR | need_args;
                }
                if (state == FileObject::OS_BUFFERED)
                {
                    std::cerr << "ERROR! rename() without fsync()! file: " << filename << " fd: " << (int)files[i].getFd() << std::endl;
                    return ERROR | need_args;
                }
#ifdef DATALOSS_DEBUG
                std::cerr << "entry clean" << std::endl;
#endif
                files[i].setName(target);
            }
        return IMPORTANT | need_args;
    }
    return OK;
}

Checker::Result DataLossChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return OK;
    string funcname = f->getName();
    uint64_t need_args = 0;

    if (funcname == "fopen" || funcname == "fopen64")
    {
        need_args = NEED_ARG0 | NEED_ARG1 | NEED_RET;
        ref<Expr> fname_exp = getCallingArgument(caller, 1, thr);
        string filename = getStrFromCharPtr(fname_exp, state);
        string mode = getStrFromCharPtr(getCallingArgument(caller, 2, thr), state);
        uint64_t fp = getIntFromExp(result);
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << filename << "," << mode << ") = " << fp << std::endl;
#endif

        return parseFOpen(filename, mode, fp) | need_args;
    } else if (funcname == "open" || funcname == "open64")
    {
        need_args = NEED_ARG0 | NEED_ARG1 | NEED_RET;
        ref<Expr> fname_exp = getCallingArgument(caller, 1, thr);
        string filename = getStrFromCharPtr(fname_exp, state);
        uint64_t flag = getIntFromExp(getCallingArgument(caller, 2, thr));
        uint64_t fd = getIntFromExp(result);
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << filename << "," << flag << ") = " << fd << std::endl;
#endif

        return parseOpen(filename, flag, fd) | need_args;
    } else if (funcname == "creat")
    {
        need_args = NEED_ARG0 | NEED_RET;
        string filename = getStrFromCharPtr(getCallingArgument(caller, 1, thr), state);
        uint64_t fd = getIntFromExp(result);
        uint64_t flag = O_WRONLY | O_CREAT | O_TRUNC;
#ifdef DATALOSS_DEBUG
        std::cerr << funcname << "(" << filename << "," << flag << ") = " << fd << std::endl;
#endif
        return parseOpen(filename, flag, fd) | need_args;
    } else if (funcname == "fdopen") {
        need_args = NEED_RET | NEED_ARG0 | NEED_ARG1;
        uint64_t fp = getIntFromExp(result);
        uint64_t fd = getIntFromExp(getCallingArgument(caller, 1, thr));
        string mode = getStrFromCharPtr(getCallingArgument(caller, 2, thr), state);
#ifdef DATALOSS_DEBUG
        std::cerr << "fdopen(" << fd << "," << mode << ") = " << fp << std::endl;
#endif
        return parseFdOpen(fd, mode, fp) | need_args;
    } else if (funcname == "fileno" || funcname == "fileno_unlocked") {
        need_args = NEED_RET | NEED_ARG0;
        ref<Expr> fp_exp = getCallingArgument(caller, 1, thr);
        uint64_t fp = getIntFromExp(fp_exp);
        uint64_t fd = getIntFromExp(result);

#ifdef DATALOSS_DEBUG
        std::cerr << "symbolic fileno(): " << fp << " -> " << fd << std::endl;
        bool found = false;
#endif
        for (unsigned int i=0; i<files.size(); i++)
        {
            if (files[i].getFp() == fp) {
                files[i].setFd(-1);
                for (unsigned int j=0; j<files.size(); j++) {
                    if (files[j].getFd() == fd) {
                        // corresponding entry, remove it
                        files.erase(files.begin() + j);
                        j--;
                    }
                }
                files[i].setFd(fd);
#ifdef DATALOSS_DEBUG
                std::cerr << "realized." << std::endl;
                found = true;
#endif
                break;
            }
        }
#ifdef DATALOSS_DEBUG
        if (!found)
        {
            std::cerr << "warning: unknown fp given to fileno()" << std::endl;
        }
#endif
        return IMPORTANT | need_args;
    }
    return OK;
}

Checker::Result DataLossChecker::report(ExecutionState *state) {
    return OK;
}

void FileObject::setState(FileState _state)
{ 
    state = _state; 
#ifdef FILEOBJ_DEBUG
    std::cerr << "transit to ";
#endif
    switch(state) {
        case CLOSED:
#ifdef FILEOBJ_DEBUG
            std::cerr << "CLOSED";
#endif
            break;
        case CLEAN:
#ifdef FILEOBJ_DEBUG
            std::cerr << "CLEAN";
#endif
            break;
        case LIB_BUFFERED:
#ifdef FILEOBJ_DEBUG
            std::cerr << "LIB_BUFFERED";
#endif
            break;
        case OS_BUFFERED:
#ifdef FILEOBJ_DEBUG
            std::cerr << "OS_BUFFERED";
#endif
            break;
        case ACCESSED:
#ifdef FILEOBJ_DEBUG
            std::cerr << "ACCESSED";
#endif
            break;
        default:
            std::cerr << "Unknown state!\n";
            assert(0);
    }
#ifdef FILEOBJ_DEBUG
    std::cerr << std::endl;
#endif
}

void FileObject::setFd(uint64_t _fd)
{
    fd = _fd;
#ifdef DATALOSS_DEBUG
    if (fd != (uint64_t)-1)
        std::cerr << "realized fd: " << fd << std::endl;
    else
        std::cerr << "lost fd" << std::endl;
#endif
}

void FileObject::setFp(uint64_t _fp)
{
    fp = _fp;
#ifdef DATALOSS_DEBUG
    if (fp != (uint64_t)-1)
        std::cerr << "realized fp: " << fp << std::endl;
    else
        std::cerr << "lost fp" << std::endl;
#endif
}

bool PairwiseChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    for (std::set<std::string>::iterator it = start_funcs->begin(); 
            it != start_funcs->end(); it++) {
        if (*it == funcname) {
            return true;
        }
    }
    for (std::set<std::string>::iterator it = end_funcs->begin(); 
            it != end_funcs->end(); it++) {
        if (*it == funcname) {
            return true;
        }
    }
    return false;
}

bool PairwiseChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE) {
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}


Checker::Result PairwiseChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE) {
    string funcname = f->getNameStr();
//    std::cerr << "Calling " << funcname << std::endl;

    if (ignore_ops) return OK;

    if (!mayBeEvent(funcname)) return OK;
    if (args != NULL) {
        for (std::set<std::string>::iterator it = start_funcs->begin();
                it != start_funcs->end(); it++) {
            if (*it == funcname) {
                uint64_t ret = args[0];
                return parseStart(*it, ret, target, thr) | NEED_RET;
            }
        }
    }

    for (std::set<std::string>::iterator it = end_funcs->begin();
            it != end_funcs->end(); it++) {
        if (*it == funcname) {
            uint64_t arg1;
            if (args != NULL) {
                arg1 = args[2];
            } else {
                arg1 = getIntFromExp(arguments[0]);
            }
            return parseEnd(*it, arg1) | NEED_ARG0;
        }
    }
    // is it reachable here?
    return OK;
}

Checker::Result PairwiseChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE) {
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return OK;
    string funcname = f->getName();
//    std::cerr << "Return " << funcname << std::endl;

    if (ignore_ops) return OK;

    if (!mayBeEvent(funcname)) return OK;

    for (std::set<std::string>::iterator it = start_funcs->begin();
            it != start_funcs->end(); it++) {
        if (*it == funcname) {
            uint64_t ret = getIntFromExp(result);
            return parseStart(*it, ret, caller, thr) | NEED_RET;
        }
    }
    // reachable here...
    return OK;
}

#define PAIRWISE_DEBUG

Checker::Result PairwiseChecker::parseStart(const std::string& func, uint64_t ret, KInstruction *inst, const ThreadState &thr) {
#ifdef PAIRWISE_DEBUG
    std::cerr << func << "() = " << ret << " thr: " << &thr << std::endl;
#endif
    if (ignore_set->count(ret)) return IMPORTANT;
    AllocInfo alloc_info(inst);
    alloc_info.addThread(ThreadInfo(thr));
    started.insert(std::pair<uint64_t, AllocInfo>(ret, alloc_info));
    return IMPORTANT;
}

Checker::Result PairwiseChecker::parseEnd(const std::string& func, uint64_t arg1) {
#ifdef PAIRWISE_DEBUG
    std::cerr << func << "(" << arg1 << ")" << std::endl;
#endif
    if (ignore_set->count(arg1)) return IMPORTANT;
    if (started.count(arg1)) {
        started.erase(arg1);
    } else {
        std::cerr << "warning: " << func << "(" << arg1 << ") without START\n";
        return ERROR;
    }
    return IMPORTANT;
}

Checker::Result PairwiseChecker::report(ExecutionState *state) {
    string place_info;
    uint64_t place_hash;
    AllocInfo::gatherPlaceInfo(state, place_info, place_hash);

    bool error = false;
    for (std::map<uint64_t, AllocInfo>::iterator it = started.begin(); it != started.end(); it++) {
        string case_info = (*it).second.toString(false);
        if (AllocInfo::isReported(*reported, place_info, place_hash, case_info)) {
            std::cerr << "ERROR SUPPRESSED: reported\n";
            continue;
        }

        if (!error) {
            std::cerr << "ERROR: resource not freed. original count: " << started.size() << std::endl;
            error = true;
        }

        std::cerr << "\tnot freed: " << (*it).first << std::endl;
        std::cerr << "\t\tallocated: " << (*it).second.toString() << std::endl;
    }
    if (error) {
        std::cerr << "Exit path: " << std::endl;
        std::cerr << place_info << std::endl;
        return ERROR;
    } else return OK;
}

AllocInfo::AllocInfo(KInstruction *_inst) {
    inst = _inst;
}

void AllocInfo::addThread(const ThreadInfo &thr) {
    threads.push_back(thr);
}

std::string ThreadInfo::callStack(bool need_args) {
    stringstream call_stack;
    const KInstruction *target = prevPC;
    call_stack << "Thread " << id << ":\n";
    for (int j=stack.size() - 1; j>=0; j--) {
        StackFrame &sf = stack[j];
        Function *f = sf.kf->function;
        const InstructionInfo &ii = *target->info;

        call_stack << "\t#" << j << ". " << std::setw(8) << ii.assemblyLine
            << " in " << f->getNameStr() << " + " << target->inst->getParent()->getNameStr() << " (";
        if (need_args) {
            unsigned index = 0;
            for (Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
                    ai != ae; ++ai) {
                if (ai != f->arg_begin()) call_stack << ", ";

                call_stack << ai->getNameStr();
                ref<Expr> value = sf.locals[sf.kf->getArgRegister(index++)].value;
                if (isa<ConstantExpr>(value)) call_stack << "=" << value;
            }
        }
        call_stack << ")";
        if (ii.file != "") call_stack << " at " << ii.file << ":" << ii.line;
        call_stack << "\n";
        target = sf.caller;
    }
    return call_stack.str();
}

std::string AllocInfo::toString(bool need_args) {
    char buf[100];

    sprintf(buf, ":%d (assembly %d) ", inst->info->line, inst->info->assemblyLine);
    stringstream alloc_site;
    alloc_site << inst->info->file << ":" << inst->info->line << " (assembly " << inst->info->assemblyLine << ")" << " BasicBlock: " << inst->inst->getParent()->getNameStr() << " Function: " << inst->inst->getParent()->getParent()->getNameStr() << "\n";
    for (unsigned int i=0; i<threads.size(); i++) {
        ThreadInfo &thr = threads[i];
        alloc_site << thr.callStack(need_args);
    }

    return alloc_site.str();
}

uint64_t AllocInfo::hash(const char *str) {
    uint64_t seed = 131;
    uint64_t hash = 0;

    while (*str) hash = hash * seed + (*str++);
    return hash;
}

ThreadInfo::ThreadInfo(const ThreadState &thr) {
    stack = thr.stack;
    id = thr.id;
    pc = thr.pc;
    prevPC = thr.prevPC;
}

bool FilePermChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "access" ||
        funcname == "open" || 
        funcname == "open64" ||
        funcname == "close")
        return true;
    return false;
}

bool FilePermChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}

#define FILEPERM_DEBUG

Checker::Result FilePermChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE
        )
{
    string funcname = f->getNameStr();
    uint64_t need_args = 0;

    if (funcname == "open" || funcname == "open64") {
        need_args = NEED_ARG0 | NEED_RET;
        string filename;
        uint64_t fd;
        if (args != NULL)
        {
            filename = string((char*)args[2]);
            fd = args[0];
#ifdef FILEPERM_DEBUG
            std::cerr << funcname << "(" << filename << "," << "mode" << ") = " << fd << std::endl;
#endif
            return parseOpen(filename, fd) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "close") {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args != NULL) {
            fd = args[2];
        } else {
            fd = getIntFromExp(arguments[0]);
        }
#ifdef FILEPERM_DEBUG
        std::cerr << funcname << "(" << fd << ")" << std::endl;
#endif
        return parseClose(fd) | need_args;
    } else if (funcname == "access" ||
            funcname == "stat" ||
            funcname == "stat64") {
        need_args = NEED_ARG0 | NEED_ARG1;
        string filename;
        int mode = 0x1481;
        if (args != NULL) {
            filename = string((char*)args[2]);
            if (funcname == "access") {
                mode = args[3];
            }
        } else {
            filename = getStrFromCharPtr(arguments[0], state);
            if (funcname == "access") {
                mode = getIntFromExp(arguments[1]);
            }
        }
#ifdef FILEPERM_DEBUG
        std::cerr << funcname << "(" << filename << ", " << mode << ")" << std::endl;
#endif
        return parseAccess(filename, mode) | need_args;
    }
    return OK;
}

Checker::Result FilePermChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Result retval = OK;
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return retval;
    string funcname = f->getName();
    uint64_t need_args = 0;

    if (funcname == "open" || funcname == "open64") {
        string filename = getStrFromCharPtr(getCallingArgument(caller, 1, thr), state);
        uint64_t fd = getIntFromExp(result);
        need_args = NEED_RET | NEED_ARG0;

#ifdef FILEPERM_DEBUG
        std::cerr << funcname << "(" << filename << "," << "mode" << ") = " << fd << std::endl;
#endif

        return parseOpen(filename, fd) | need_args;
    }

    return OK;
}

Checker::Result FilePermChecker::parseAccess(const std::string &filename, int mode) {
    if (mode == R_OK)
        return OK;
    if (files.count(filename) > 0) {
        // old file
        vector<FileObject*>& file_list = files[filename];
        for (unsigned int i=0; i<file_list.size(); i++) {
            if (file_list[i]->getState() == FileObject::CLEAN) {
                // access() after open()!
                std::cerr << "access() after open()!" << std::endl;
                return ERROR;
            }
        }
        assert(file_list.size() == 1);
        assert(file_list[0]->getState() == FileObject::ACCESSED);
        // else, there must be instance with ACCESSED
    } else {
        // new file
        FileObject *obj = new FileObject(filename, -1);
        obj->setState(FileObject::ACCESSED);
        vector<FileObject *> file_list;
        file_list.push_back(obj);
        files[filename] = file_list;
    }
    return IMPORTANT;
}

Checker::Result FilePermChecker::parseOpen(const std::string &filename, uint64_t fd) {
    if (files.count(filename) > 0) {
        // old file
        vector<FileObject*> file_list = files[filename];
        for (unsigned int i=0; i<file_list.size(); i++) {
            if (file_list[i]->getState() == FileObject::ACCESSED) {
                std::cerr << "open() after access()!" << std::endl;
                return ERROR;
            }
            assert(file_list[i]->getState() == FileObject::CLEAN);
        }
        // other opens, don't care
    }
    // new file
    FileObject *obj = new FileObject(filename, -1, fd);
    obj->setState(FileObject::CLEAN);
    if (files.count(filename) > 0) {
        files[filename].push_back(obj);
    } else {
        vector<FileObject*> file_list;
        file_list.push_back(obj);
        files[filename] = file_list;
    }

    if (files_by_fd.count(fd)) {
        if (files_by_fd[fd]->getState() == FileObject::CLEAN) {
            std::cerr << "error! inconsistency in library: same fd opened again! fd: " << fd << std::endl;
        }
    }
    files_by_fd[fd] = obj;
    return IMPORTANT;
}

Checker::Result FilePermChecker::parseClose(uint64_t fd) {
    if (files_by_fd.count(fd) > 0) {
        FileObject *obj = files_by_fd[fd];
        const std::string & filename = obj->getName();
        // internal consistency
        assert(files.count(filename));
        vector<FileObject *> &named_files = files[filename];
        if (named_files.size() == 1) {
            // internal cosistency
            assert(named_files[0] == obj);
            files.erase(filename);
        } else {
            for (unsigned int i=0; i<named_files.size(); i++) {
                if (named_files[i] == obj) {
                    named_files.erase(named_files.begin() + i);
                    break;
                }
            }
        }
        delete obj;
    } else {
        std::cerr << "warning: closed file is not in the records" << std::endl;
    }

    return IMPORTANT;
}

FilePermChecker::~FilePermChecker() {
#ifdef FILEPERM_MEM_DEBUG
    std::cerr << "enter destructor\n";
#endif
    for (std::map<std::string, std::vector<FileObject *> >::iterator it = files.begin();
            it != files.end(); it++) {
        for (unsigned int i=0; i<it->second.size(); i++) {
#ifdef FILEPERM_MEM_DEBUG
            std::cerr << hex << "deleting " << it->second[i] << std::endl;
#endif
            delete it->second[i];
        }
    }
}

FilePermChecker::FilePermChecker(const FilePermChecker& other) : Checker(other) {
    for (std::map<std::string, std::vector<FileObject*> >::const_iterator it = other.files.begin();
            it != other.files.end(); it++) {
        vector<FileObject*> my_vec;
        for (unsigned int i=0; i<it->second.size(); i++) {
            FileObject *obj = new FileObject(*it->second[i]); 
            my_vec.push_back(obj);
            if (obj->getFd() != (uint64_t)-1) {
                files_by_fd[obj->getFd()] = obj;
            }
        }
        files[it->first] = my_vec;
    }
}

std::string LeakChecker::allocaters[] = {"malloc", "calloc"};
std::string LeakChecker::terminators[] = {"free"};
uint64_t LeakChecker::ignore_set[] = {0};

LeakChecker::LeakChecker(Executor *executor) : 
    PairwiseChecker(executor, 
            std::set<std::string>(allocaters, allocaters + sizeof(allocaters) / sizeof(allocaters[0])), 
            std::set<std::string>(terminators, terminators + sizeof(terminators) / sizeof(terminators[0])),
            std::set<uint64_t>(ignore_set, ignore_set + sizeof(ignore_set) / sizeof(ignore_set[0]))) {
}

bool LeakChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "realloc" ||
        funcname == "strdup" ||
        funcname == "strndup" ||
        funcname == "getline"
        ) return IMPORTANT;
    return PairwiseChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS);
}

bool LeakChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE)
{
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}

Checker::Result LeakChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE
        )
{
    string funcname = f->getNameStr();
    uint64_t need_args;

    if (funcname == "init_cur_collate") ignore_ops++;
    if (funcname == "__fd_open") ignore_ops++;
    if (funcname == "write") ignore_ops++;
    if (funcname == "_stdio_fopen") ignore_ops++;
    if (funcname == "fclose") ignore_ops++;
    if (funcname == "klee_init_env") ignore_ops++;

    if (ignore_ops) return OK;

    if (funcname == "realloc") {
        need_args = NEED_RET | NEED_ARG0 | NEED_ARG1;
        if (args != NULL) {
            uint64_t ptr = args[2];
            uint64_t size = args[3];
            uint64_t ret = args[0];
#ifdef FORK_CALL_DEBUG
            std::cerr << "concrete Realloc(" << ptr << "," << size << ") = " << ret << std::endl;
#endif

            return parseRealloc(ptr, size, ret, target, thr) | need_args;
        }
        return IMPORTANT | need_args;
    }

    if (funcname == "getline") {
        need_args = NEED_ARG0;
        if (args == NULL) {
            ref<Expr> lineptr = arguments[0];
//            ref<Expr> fstream = arguments[2];

            uint64_t lineaddr = getPtrFromPPtr(lineptr, state);
            uint64_t *curargs = new uint64_t[1];
            curargs[0] = lineaddr;
            callargs.insert(make_pair<int, uint64_t*>(thr.id, curargs));
//            std::cerr << "getline(&" << lineaddr << ") " << curargs << "\n";
        } else {
            // since we need calling & returning args
            // we can't process this case
            std::cerr << "WARNING: cannot process external getline() calls correctly" << std::endl;
        }
        ignore_ops = true;
        return IMPORTANT | need_args;
    }

    if (funcname == "strdup" || funcname == "strndup") {
        need_args = NEED_RET;
        if (args) {
            uint64_t ret = args[0];
            if (ret) {
                parseStart(funcname, ret, target, thr);
            }
        }
        return IMPORTANT | need_args;
    }

    return PairwiseChecker::onCall(state, thr, CHECKER_ONCALL_ARGUMENTS);
}

Checker::Result LeakChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Result retval = OK;
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f)
        return retval;
    string funcname = f->getName();

    if (funcname == "init_cur_collate") ignore_ops--;
    if (funcname == "__fd_open") ignore_ops--;
    if (funcname == "write") ignore_ops--;
    if (funcname == "_stdio_fopen") ignore_ops--;
    if (funcname == "fclose") ignore_ops--;
    if (funcname == "klee_init_env") ignore_ops--;
    if (funcname == "getline") ignore_ops--;

    if (ignore_ops) return OK;

    uint64_t need_args = 0;
    if (funcname == "realloc") {
        uint64_t ret = getIntFromExp(result);
        uint64_t ptr = getIntFromExp(getCallingArgument(caller, 1, thr));
        uint64_t size = getIntFromExp(getCallingArgument(caller, 2, thr));
        need_args = NEED_RET | NEED_ARG0 | NEED_ARG1;

#ifdef FORK_CALL_DEBUG
        std::cerr << "symbolic Realloc(" << ptr << "," << size << ") = " << ret << std::endl;
#endif
        return parseRealloc(ptr, size, ret, caller, thr) | need_args;
    }

    if (funcname == "getline") {
        uint64_t lineaddr = getPtrFromPPtr(getCallingArgument(caller, 1, thr), state);
        need_args = NEED_ARG0;
        uint64_t *enterargs = callargs[thr.id];
        callargs.erase(thr.id);
        if (enterargs == NULL) {
            std::cerr << "ERROR! onRet(getline) without onCall(getline)!" << std::endl;
            return IMPORTANT | need_args;
        }
        uint64_t lastlineaddr = enterargs[0];
        if (lastlineaddr == (uint64_t)-1) {
            return IMPORTANT | need_args;
        }
        if (lineaddr == (uint64_t)-1) {
            return IMPORTANT | need_args;
        }
        if (lastlineaddr != lineaddr) {
//            std::cerr << "getline(&" << lastlineaddr << ") = &" << lineaddr << " " << enterargs << std::endl;
            if (lastlineaddr != 0) {
                parseEnd("getline", lastlineaddr);
            }
            // lastlineaddr != 0: realloc
            parseStart("getline", lineaddr, caller, thr);
        } // same: nothing changed
        return IMPORTANT | need_args;
    }

    if (funcname == "strdup" || funcname == "strndup") {
        need_args = NEED_RET;
        uint64_t ret = getIntFromExp(result);
        if (ret) {
            parseStart(funcname, ret, caller, thr);
        }
        return IMPORTANT | need_args;
    }

    return PairwiseChecker::onRet(state, thr, CHECKER_ONRET_ARGUMENTS);
}

Checker::Result LeakChecker::report(ExecutionState *state) {
    return PairwiseChecker::report(state);
}

Checker::Result LeakChecker::parseRealloc(uint64_t ptr, uint64_t size, uint64_t ret, KInstruction *inst, const ThreadState &thr) {
    if (ptr == 0) {
        // ptr==0: malloc(size)
        return parseStart("realloc", ret, inst, thr);
    }
    // ptr != 0
    if (size == 0) {
        // size==0: free(ptr)
        return parseEnd("realloc", ptr);
    }
    // size != 0 && ptr != 0
    if (ret == 0) {
        // nothing changed...
        return IMPORTANT;
    } else {
        // ptr is freed
        parseEnd("realloc", ptr);
        // ret is newly allocated
        parseStart("realloc", ret, inst, thr);
        return IMPORTANT;
    }
}

bool SysOpenCloseChecker::mayBeEvent(CHECKER_MAYBEEVENT_ARGUMENTS_WITH_TYPE) {
    if (funcname == "open" ||
            funcname == "close") {
        return true;
    }
#ifdef WITHOUT_LIB_CALLS
    if (funcname == "fdopen" ||
            funcname == "fclose") {
        return true;
    }
#endif

    if (!written_only) {
        if (funcname == "write") {
            return true;
        }

#ifdef WITHOUT_LIB_CALLS
        if (funcname == "fputs" ||
        funcname == "fputs_unlocked" ||
        funcname == "fprintf" ||
        funcname == "fwrite" ||
        funcname == "fwrite_unlocked") {
            return true;
        }
#endif
    }

    return false;
}

bool SysOpenCloseChecker::wantConcrete(ExecutionState &state,
        ThreadState &thr,
        CHECKER_WANTCONCRETE_ARGUMENTS_WITH_TYPE) {
    string funcname = f->getNameStr();
    return mayBeEvent(funcname);
}


Checker::Result SysOpenCloseChecker::onCall(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONCALL_ARGUMENTS_WITH_TYPE
        )
{
    string funcname = f->getNameStr();
    uint64_t need_args = 0;

    if (funcname == "open") {
        need_args = NEED_RET;
        uint64_t fd;
        if (args) {
            fd = args[0];
            AllocInfo alloc_info(target);
            alloc_info.addThread(ThreadInfo(thr));
            return parseOpen(fd, alloc_info) | need_args;
        }
        return IMPORTANT | need_args;
    } else if (funcname == "close") {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args) {
            fd = args[2];
        } else {
            fd = getIntFromExp(arguments[0]);
        }
        return parseClose(fd) | need_args;
    } else if (funcname == "write") {
        need_args = NEED_ARG0;
        uint64_t fd;
        if (args) {
            fd = args[2];
        } else {
            fd = getIntFromExp(arguments[0]);
        }
        return parseWrite(fd) | need_args;
    }
#ifdef WITHOUT_LIB_CALLS
    else if (funcname == "fdopen") {
        need_args = NEED_ARG0 | NEED_RET;
        uint64_t fp, fd;
        if (args) {
            fp = args[0];
            fd = args[2];
            return parseFdopen(fd, fp) | need_args;
        }
        return IMPORTANT | need_args;
    }
#endif
    return OK;
}

Checker::Result SysOpenCloseChecker::onRet(ExecutionState &state,
        ThreadState &thr,
        CHECKER_ONRET_ARGUMENTS_WITH_TYPE
        )
{
    Function *f = getTargetFunctionFromCaller(caller, state);
    if (!f) {
        log.warn("fail to decide target function in onRet(). ignore.");
        return OK;
    }
    string funcname = f->getName();
    uint64_t need_args;

    if (funcname == "open") {
        uint64_t fd = getIntFromExp(result);
        need_args = NEED_RET;
        AllocInfo alloc_info(caller);
        alloc_info.addThread(ThreadInfo(thr));
        return parseOpen(fd, alloc_info) | need_args;
    }
    return OK;
}

Checker::Result SysOpenCloseChecker::parseOpen(int fd, AllocInfo &alloc_info) {
    if (fd != -1) {
        FileObject fo("", -1, fd);
        fo.setState(FileObject::CLEAN);
        fo.setAllocInfo(alloc_info);
        opened.insert(make_pair(fd, fo));
    } else {
        log.warn("open failed");
    }
    return IMPORTANT;
}

Checker::Result SysOpenCloseChecker::parseWrite(int fd) {
    if (fd != -1) {
        map<uint64_t, FileObject>::iterator ifo = opened.find(fd);
        if (ifo == opened.end()) {
            log.warn("write to unknown file");
            return IMPORTANT;
        }
        FileObject &fo = ifo->second;
        fo.setState(FileObject::OS_BUFFERED);
    } else {
        log.warn("illegal write: fd = %d", fd);
    }
    return IMPORTANT;
}

Checker::Result SysOpenCloseChecker::parseClose(int fd) {
    if (fd != -1) {
        map<uint64_t, FileObject>::iterator ifo = opened.find(fd);
        if (ifo == opened.end()) {
            log.warn("close an unknown file");
            return IMPORTANT;
        }
        opened.erase(ifo);
    } else {
        log.warn("illegal close: fd = %d", fd);
    }
    return IMPORTANT;
}

#ifdef WITHOUT_LIB_CALLS
Checker::Result SysOpenCloseChecker::parseFdopen(uint64_t fd, uint64_t fp) {
}
#endif

Checker::Result SysOpenCloseChecker::report(ExecutionState *state) {
    string place_info;
    uint64_t place_hash;
    AllocInfo::gatherPlaceInfo(state, place_info, place_hash);
//    cout << "=== PATH END ===" << endl;
    int error = 0;
    for (map<uint64_t, FileObject>::iterator it = opened.begin(); it != opened.end(); it++)
    {
        string case_info = it->second.getAllocInfo().toString(false);
        if (AllocInfo::isReported(reported, place_info, place_hash, case_info)) {
            log.info("ERROR SUPPRESSED: reported");
            continue;
        }

        if (written_only) {
            if (it->second.getState() == FileObject::CLEAN) {
                log.debug("ERROR SUPPRESSED: not written");
                continue;
            }
        }

        error++;
        log.err("not closed: %d", it->second.getFd());
        log.err("\topened at: \n%s", it->second.getAllocInfo().toString().c_str());
    }
    if (error)
        log.err("fd not closed! count: %d (orig: %d)", error, opened.size());
    if (error)
        return ERROR;
    else
        return OK;
}

std::multimap<uint64_t, std::string> SysOpenCloseChecker::reported;

SysOpenCloseChecker::~SysOpenCloseChecker() {
}
