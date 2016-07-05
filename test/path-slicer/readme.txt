1 Build a test file.
cd $DIRECT_SYM_ROOT/tests/
./build assert.c

2 Run a test.
$DIRECT_SYM_ROOT/klee/Debug+Asserts/bin/klee --max-time 15 --libc=uclibc \
--posix-runtime --init-env ./assert.bc --sym-args 1 10 10 --sym-files 2 20 \
--max-fail 1

3 Look at a test file (which contains an assertion failure).
$DIRECT_SYM_ROOT/klee/Debug+Asserts/bin/ktest-tool ./klee-last/test000011.ktest

