LEVEL = $(shell $(LLVM_ROOT)/scripts/level-to-llvm-root)/llvm-obj/

LIBRARYNAME := dsym-print-callgraph
LOADABLE_MODULE := 1

include $(LEVEL)/Makefile.common

CXXFLAGS += -I $(LLVM_ROOT)/common/include/

#LIBS += $(LLVM_ROOT)/install/lib/libcallgraph-fp.so

#LIBS += $(LLVM_ROOT)/install/lib/libcallgraph-fp.so \
        $(LLVM_ROOT)/install/lib/libbc2bdd.so \
        -lbdd

clean::
	rm -rf Debug
	rm -rf Release
