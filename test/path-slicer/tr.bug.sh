$DIRECT_SYM_ROOT/klee/Debug+Asserts/bin/klee --max-time 200 --libc=uclibc --posix-runtime \
--init-env $DIRECT_SYM_ROOT/tests/tr.bc --sym-args 1 10 10 --sym-files 2 2000 --max-fail 1
