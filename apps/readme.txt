1 Stack space.
  Since KLEE uses much more stack space than x86, please add "ulimit -s 819200"
to your "~/.bashrc" so that KLEE would not run out of stack.
