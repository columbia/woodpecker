#===-- lib/Core/Makefile -----------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

LIBRARYNAME=kleeCore
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1

include $(LEVEL)/Makefile.common

CXXFLAGS += -I$(XTERN_ROOT)/include/ -I$(LLVM_ROOT) \
	-I$(LLVM_ROOT)/common/include/ \
        -I$(DIRECT_SYM_ROOT)/klee/lib/stp/install/include/ -I$(XTERN_ROOT)/lib/
