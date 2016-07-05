1 Stack capacity. Sometimes when running the original KLEE with coreutils on some
  machines would trigger a "KLEE: WARNING: flushing 255904 bytes on read, may be 
slow and/or crash: MO183[255904] allocated at global:locale_mmap", and a segmentation
fault. This is because the stack of Linux is not big enough for KLEE, run the 
"ulimit -s 819200" to enlarge the stack size for 100x would solve the problem.

2 After the "./mk-llvm $VERSION" is done, before you run the evaluation scripts,
manually copy the "charset.alias" to /usr/local/lib/, since we need this file to reach some file events.
> sudo cp $DIRECT_SYM_ROOT/apps/coreutils/coreutils-$VERSION/obj-llvm/lib/charset.alias /usr/local/lib/

3 TBD.
