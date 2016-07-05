#ifndef KLEE_BASICCHECKER_H
#define KLEE_BASICCHECKER_H

#include "klee/Checker.h"

#include "klee/Expr.h"
#include "klee/ExecutionState.h"
#include <set>
#include "Log.h"

namespace klee {
    class KInstruction;

    class FileObject;

class AssertCounter : public Checker {
private:
        static std::set<KInstruction *> asserts, nonAsserts;
        int constBranch, symBranch, assertCount, 
                   symAssert, realFork, assertFork, branchCount;

public:
        DECLARE_EVENT_HANDLER(onBranch, ONBRANCH);

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new AssertCounter(*this); }

        AssertCounter(Executor *executor);
};

class AssertChecker : public Checker {
private:
        static std::set<KInstruction *> asserts, nonAsserts;

public:
        DECLARE_EVENT_HANDLER(onBranch, ONBRANCH);

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new AssertChecker(*this); }

        AssertChecker(Executor *executor);
};

class ThreadInfo {
public:
    std::vector<StackFrame> stack;
    int id;
    KInstIterator pc, prevPC;
    ThreadInfo(const ThreadState &thr);
    std::string callStack(bool need_args);
};

class AllocInfo {
        KInstruction *inst;
        std::vector<ThreadInfo> threads;

public:
        AllocInfo(KInstruction *_inst = NULL);
        void addThread(const ThreadInfo &thr);
        std::string toString(bool need_args = true);
        static uint64_t hash(const char*);
        static bool isReported(std::multimap<uint64_t, std::string>& record, const std::string& place_info, uint64_t place_hash, const std::string& case_info);
        static void gatherPlaceInfo(ExecutionState *state, std::string &place_info, uint64_t &place_hash);
};

class OpenCloseChecker : public Checker {
private:
        std::set<uint64_t> open_fds;
        std::map<uint64_t, AllocInfo> open_fds_allocinfo;
        std::set<ref<Expr> > open_fds_exp;
        std::map<Expr*, AllocInfo> open_fds_exp_allocinfo;
        bool dismiss_error;
        static std::multimap<uint64_t, std::string> reported;

public:
        OpenCloseChecker(Executor *executor) : Checker(executor), dismiss_error(false) {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        DECLARE_EVENT_HANDLER(onRet, ONRET);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new OpenCloseChecker(*this); }
};

class FileChecker : public Checker {
private:
        std::set<uint64_t> open_fds;
        std::set<uint64_t> err_fds;
        std::set<uint64_t> dirty_fds, err_dirty_fds;

public:
        FileChecker(Executor *executor) : Checker(executor) {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        DECLARE_EVENT_HANDLER(onRet, ONRET);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new FileChecker(*this); }
};

class LockChecker : public Checker {
private:
        std::set<uint64_t> init_locks;
        std::set<uint64_t> locked_locks;

public:
        LockChecker(Executor *executor) : Checker(executor) {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new LockChecker(*this); }
};

class DataLossChecker : public Checker {
private:
        std::vector<FileObject> files;
        std::set<std::string> unlinked_files;

        Checker::Result parseOpen(const std::string &filename, uint64_t flag, uint64_t fd);
        Checker::Result parseFOpen(const std::string &filename, const std::string &mode, uint64_t fp);
        Checker::Result parseFdOpen(uint64_t fd, const std::string &mode, uint64_t fp);
public:
        DataLossChecker(Executor *executor) : Checker(executor) {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);

        DECLARE_EVENT_HANDLER(onRet, ONRET);

        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        virtual Checker::Result report(ExecutionState *state);

        virtual Checker* clone() { return new DataLossChecker(*this); }
};

class FileObject {
public:
    enum FileState {
        CLOSED,
        CLEAN,
        ACCESSED,
        LIB_BUFFERED,
        OS_BUFFERED
    };

private:
    std::string filename;
    uint64_t fp;
    uint64_t fd;

    FileState state;
    AllocInfo alloc_info;

public:
    FileObject(const std::string& _filename, uint64_t _fp, uint64_t _fd = -1)
        : filename(_filename), fp(_fp), fd(_fd), state(CLOSED) {
        }

    uint64_t getFp() { return fp; }
    void setFp(uint64_t _fp);
    uint64_t getFd() { return fd; }
    void setFd(uint64_t _fd);
    const std::string& getName() { return filename; }
    void setName(const std::string& _name) { filename = _name; }
    FileState getState() { return state; }
    void setState(FileState _state);
    AllocInfo getAllocInfo() { return alloc_info; }
    void setAllocInfo(const AllocInfo &_alloc_info) { alloc_info = _alloc_info; }
};

class PairwiseChecker : public Checker {
private:
        std::map<uint64_t, AllocInfo> started;
        std::set<std::string> *start_funcs, *end_funcs;
        std::set<uint64_t> *ignore_set;
        std::multimap<uint64_t, std::string> *reported;

protected:
        virtual Checker::Result parseStart(const std::string& func, uint64_t ret, KInstruction *inst, const ThreadState &thr);
        virtual Checker::Result parseEnd(const std::string& func, uint64_t arg1);
        int ignore_ops;

public:
        // Simple case
        PairwiseChecker(Executor *executor, const std::string& _start_func,
                const std::string& _end_func) : Checker(executor) {
            start_funcs = new std::set<std::string>();
            start_funcs->insert(_start_func);
            end_funcs = new std::set<std::string>();
            end_funcs->insert(_end_func);
            ignore_set = new std::set<uint64_t>();
            ignore_ops = 0;
            reported = new std::multimap<uint64_t, std::string>();
        }

        // Complex case
        PairwiseChecker(Executor *executor, const std::set<std::string> &_start_funcs,
                const std::set<std::string> &_end_funcs, const std::set<uint64_t> &_ignore_set) : Checker(executor)
        {
            start_funcs = new std::set<std::string>(_start_funcs);
            end_funcs = new std::set<std::string>(_end_funcs);
            ignore_set = new std::set<uint64_t>(_ignore_set);
            ignore_ops = 0;
            reported = new std::multimap<uint64_t, std::string>();
        }

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);
        DECLARE_EVENT_HANDLER(onCall, ONCALL);
        DECLARE_EVENT_HANDLER(onRet, ONRET);
        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        virtual void freeFuncs() {
            delete start_funcs;
            delete end_funcs;
            delete ignore_set;
            delete reported;
            start_funcs = 0;
            end_funcs = 0;
            ignore_set = 0;
            reported = NULL;
        }

        virtual Checker::Result report(ExecutionState *state);
        virtual Checker* clone() { return new PairwiseChecker(*this); }
};

class LeakChecker : public PairwiseChecker {
private:
        // realloc() is much more complex
        static std::string allocaters[];
        static std::string terminators[];
        static uint64_t ignore_set[];
        std::map<int, uint64_t*> callargs;

protected:
        virtual Checker::Result parseRealloc(uint64_t ptr, uint64_t size, uint64_t ret, KInstruction *inst, const ThreadState &thr);

public:
        LeakChecker(Executor *executor);
        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);
        DECLARE_EVENT_HANDLER(onCall, ONCALL);
        DECLARE_EVENT_HANDLER(onRet, ONRET);
        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);
        virtual Checker::Result report(ExecutionState *state);
        virtual Checker* clone() { return new LeakChecker(*this); }
};

class FilePermChecker : public Checker {
private:
        std::map<std::string, std::vector<FileObject*> > files;
        std::map<uint64_t, FileObject*> files_by_fd;

protected:
        virtual Checker::Result parseAccess(const std::string& filename, int mode);
        virtual Checker::Result parseOpen(const std::string& filename, uint64_t fd);
        virtual Checker::Result parseClose(uint64_t fd);

public:
        FilePermChecker(Executor *executor) : Checker(executor) {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);
        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);
        DECLARE_EVENT_HANDLER(onRet, ONRET);

//        virtual Checker::Result report(ExecutionState *state);
        virtual Checker* clone() { return new FilePermChecker(*this); }
        virtual ~FilePermChecker();
        FilePermChecker(const FilePermChecker &other);
};

class SysOpenCloseChecker : public Checker {
private:
        std::map<uint64_t, FileObject> opened;
        bool written_only;
        Log log;
        static std::multimap<uint64_t, std::string> reported;

protected:
        virtual Checker::Result parseOpen(int fd, AllocInfo &alloc_info);
        virtual Checker::Result parseWrite(int fd);
        virtual Checker::Result parseClose(int fd);

public:
        SysOpenCloseChecker(Executor *executor, bool _written_only) : Checker(executor), written_only(_written_only), log("SysOpenCloseChecker") {}

        DECLARE_STATIC_PROP_HANDLER(mayBeEvent, MAYBEEVENT);
        DECLARE_PROP_HANDLER(wantConcrete, WANTCONCRETE);

        DECLARE_EVENT_HANDLER(onCall, ONCALL);
        DECLARE_EVENT_HANDLER(onRet, ONRET);

        virtual Checker* clone() { return new SysOpenCloseChecker(*this); }
        virtual ~SysOpenCloseChecker();
        virtual Checker::Result report(ExecutionState *state);

};

}

#endif // KLEE_BASICCHECKER_H
