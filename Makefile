#===-- Makefile ---------------------------------------*- Makefile -*--===#
#
#                     DIRECT-SYM
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common
CXXFLAGS += -frtti

all:: uclibc stp klee instruments

uclibc:: uclibc-config uclibc-make

uclibc-config-file: $(DIRECT_SYM_ROOT)/klee/uclibc/.config $(DIRECT_SYM_ROOT)/klee/uclibc/libc/sysdeps/linux/$(ARCH)/bits/kernel_types.h

$(DIRECT_SYM_ROOT)/klee/uclibc/.config:
	cp $(DIRECT_SYM_ROOT)/klee/uclibc/src-dep/.config-$(ARCH) $(DIRECT_SYM_ROOT)/klee/uclibc/.config

$(DIRECT_SYM_ROOT)/klee/uclibc/libc/sysdeps/linux/$(ARCH)/bits/kernel_types.h:
	cp $(DIRECT_SYM_ROOT)/klee/uclibc/src-dep/kernel_types-$(ARCH).h $(DIRECT_SYM_ROOT)/klee/uclibc/libc/sysdeps/linux/$(ARCH)/bits/kernel_types.h

uclibc-config: uclibc-config-file uclibc-configure

uclibc-configure: $(DIRECT_SYM_ROOT)/klee/uclibc/Rules.mak

$(DIRECT_SYM_ROOT)/klee/uclibc/Rules.mak:
	cd $(DIRECT_SYM_ROOT)/klee/uclibc && ./configure --with-llvm=$(LLVM_ROOT)

uclibc-make:
	cd $(DIRECT_SYM_ROOT)/klee/uclibc && make LLVMGCC=llvm-gcc

stp::
	cd $(DIRECT_SYM_ROOT)/klee/lib/stp && make && make install

klee:: klee-configure klee-make

klee-configure: $(DIRECT_SYM_ROOT)/klee/Makefile.config

$(DIRECT_SYM_ROOT)/klee/Makefile.config:
	cd $(DIRECT_SYM_ROOT)/klee && ./configure \
		--with-llvmsrc=$(LLVM_ROOT)/llvm-2.7/ \
		--with-llvmobj=$(LLVM_ROOT)/llvm-obj \
		--with-llvm-build-mode=$(BuildMode) \
		--with-runtime=$(BuildMode) \
		--with-uclibc=$(DIRECT_SYM_ROOT)/klee/uclibc \
		--enable-posix-runtime \
		--with-stp=$(DIRECT_SYM_ROOT)/klee/lib/stp/install/

klee-make:
	cd $(DIRECT_SYM_ROOT)/klee && make DISABLE_ASSERTIONS=0

instruments::
	cd $(DIRECT_SYM_ROOT)/instruments && make

test::
	cd tests
	make

clean::
	cd $(DIRECT_SYM_ROOT)/klee && rm -rf Debug && rm -rf Release && rm -rf uclibc/lib && make clean

