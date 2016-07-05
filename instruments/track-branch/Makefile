LEVEL = $(shell $(LLVM_ROOT)/scripts/level-to-llvm-root)/llvm-obj/

LIBRARYNAME := dsym-track-branch
LOADABLE_MODULE := 1

include $(LEVEL)/Makefile.common

CXXFLAGS += -I $(LLVM_ROOT)/common/include/

func::
	cd func && make

clean::
	rm -rf Debug
	rm -rf Release

