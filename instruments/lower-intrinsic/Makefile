LEVEL = $(shell $(LLVM_ROOT)/scripts/level-to-llvm-root)/llvm-obj
LIBRARYNAME = dsym-lower-intrinsic
LOADABLE_MODULE = 1
BUILD_ARCHIVE = 1

#LINK_COMPONENTS = codegen

include $(LEVEL)/Makefile.common

# Heming: currently have to link in this .o, not the codegen component, because it would report a DominatorTree multiple registration problem.
LIBS = \
	$(LLVM_ROOT)/llvm-obj/lib/CodeGen/$(BuildMode)/IntrinsicLowering.o

clean::
	rm -rf Debug
	rm -rf Release
