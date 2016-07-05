1 Stack capacity. Sometimes when running the original KLEE with coreutils on some
  machines would trigger a "KLEE: WARNING: flushing 255904 bytes on read, may be 
slow and/or crash: MO183[255904] allocated at global:locale_mmap", and a segmentation
fault. This is because the stack of Linux is not big enough for KLEE, run the 
"ulimit -s 819200" to enlarge the stack size for 100x would solve the problem.

2 TBD.
