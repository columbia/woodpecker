dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[[DIRECT-SYM]]],[[[0.10]]],[heming@cs.columbia.edu])

dnl Identify where LLVM source tree is (this is patched by
dnl AutoRegen.sh)
LLVM_SRC_ROOT=XXX

dnl Tell autoconf that the auxilliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR($LLVM_SRC_ROOT/autoconf)

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT("","")

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.config.in"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.config)

dnl Configure a header file
AC_CONFIG_HEADERS([include/direct-sym/config.h])

dnl FIXME: Make out of tree builds work.

AC_PROG_CPP
AC_PROG_CC(gcc)
AC_PROG_CXX(g++)

dnl **************************************************************************
dnl Verify that we can find llvm

dnl --with-llvm is a shortcut for setting srcdir and objdir.
AC_ARG_WITH(llvm,
  AS_HELP_STRING([--with-llvm],
    [Location of LLVM Source and Object code]),,)

AC_MSG_CHECKING([llvm source dir])

if test X${with_llvm} != X; then
    dnl Verify that --with-llvm{src,obj} were not given.
    if test X${with_llvmsrc} != X; then
       AC_MSG_ERROR([--with-llvmsrc cannot be specified when using --with-llvm])
    fi   
    if test X${with_llvmobj} != X; then
       AC_MSG_ERROR([--with-llvmobj cannot be specified when using --with-llvm])
    fi   
    with_llvmsrc=$with_llvm
    with_llvmobj=$with_llvm
fi

dnl If one of with_llvmsrc or with_llvmobj was given, we must have both.
if (test X${with_llvmsrc} != X || test X${with_llvmobj} != X); then
    dnl Verify that with_llvmobj was given as well.
    if test X${with_llvmsrc} = X; then
       AC_MSG_ERROR([--with-llvmsrc must be specified when using --with-llvmobj])
    fi      
    if test X${with_llvmobj} = X; then
       AC_MSG_ERROR([--with-llvmobj must be specified when using --with-llvmsrc])
    fi      
else
    dnl Otherwise try and use llvm-config to find.
    llvm_version=`llvm-config --version`
    if test X${llvm_version} = X; then
       AC_MSG_ERROR([unable to find llvm, use --with-llvmsrc and --with-llvmobj])
    fi
    
    with_llvmsrc=`llvm-config --src-root`
    with_llvmobj=`llvm-config --obj-root`
fi

dnl Try to validate directories.
if test ! -f ${with_llvmsrc}/Makefile.rules; then
   AC_MSG_ERROR([invalid llvmsrc directory: ${with_llvmsrc}])
fi
if test ! -f ${with_llvmobj}/Makefile.config; then
   AC_MSG_ERROR([invalid llvmobj directory: ${with_llvmobj}])
fi

dnl Make the paths absolute.
llvm_src=`cd $with_llvmsrc 2> /dev/null; pwd`
llvm_obj=`cd $with_llvmobj 2> /dev/null; pwd`
   
AC_MSG_RESULT([$llvm_src])

dnl Report obj dir as well.
AC_MSG_CHECKING([llvm obj dir])
AC_MSG_RESULT([$llvm_obj])

AC_SUBST(LLVM_SRC,$llvm_src)
AC_SUBST(LLVM_OBJ,$llvm_obj)

dnl Allow a specific llvm-gcc/llvm-g++/opt triple to be used with this 
dnl LLVM config.
AC_ARG_WITH(llvmgccdir,
  AS_HELP_STRING([--with-llvmbindir],
    [Specify location of llvm-gcc/llvm-g++/opt install dir (default to llvm-config --bindir)]),,
    withval=default)
case "$withval" in
  default) WITH_LLVMBINDIR=`llvm-config --bindir` ;;
  /* | [[A-Za-z]]:[[\\/]]*)      WITH_LLVMBINDIR=$withval ;;
  *) AC_MSG_ERROR([Invalid path for --with-llvmbindir. Provide full path]) ;;
esac

AC_MSG_CHECKING([llvm bin dir])
AC_MSG_RESULT([$WITH_LLVMBINDIR])

LLVMGCC="$WITH_LLVMBINDIR/llvm-gcc"
LLVMGXX="$WITH_LLVMBINDIR/llvm-g++"
LOPT="$WITH_LLVMBINDIR/opt"
LLVMLD="$WITH_LLVMBINDIR/llvm-ld"

dnl Validate llvm-gcc/llvm-g++/opt triple
if test ! -f "${LLVMGCC}"; then
   AC_MSG_ERROR([Unable to find llvm-gcc.  Use --with-llvmbindir]);
fi
if test ! -f "$LLVMGXX"; then
   AC_MSG_ERROR([Unable to find llvm-g++.  Use --with-llvmbindir]);
fi
if test ! -f "$LOPT"; then
   AC_MSG_ERROR([Unable to find opt.  Use --with-llvmbindir]);
fi
if test ! -f "$LLVMLD"; then
   AC_MSG_ERROR([Unable to find llvm-ld.  Use --with-llvmbindir]);
fi

AC_SUBST(LLVMCC_OPTION,llvm-gcc)
AC_SUBST(LLVMGCC,$LLVMGCC)
AC_SUBST(LLVMGXX,$LLVMGXX)
AC_SUBST(LOPT,$LOPT)
AC_SUBST(LLVMLD,$LLVMLD)

dnl AC_ARG_ENABLE(atexit, 
dnl              AS_HELP_STRING([--enable-atexit],
dnl              [Schedule tern runtime shutdown code using atexit()]),
dnl              ,enableval="no")
dnl AC_MSG_CHECKING([Enable atexit()])
dnl if test ${enableval} = "no" ; then
dnl   AC_SUBST(ENABLE_ATEXIT,[[]])
dnl else
dnl   AC_SUBST(ENABLE_ATEXIT,[[ENABLE_ATEXIT=1]])
dnl   AC_DEFINE(ENABLE_ATEXIT, [1], 
dnl   [Installing tern runtime teardown code using atexit() (default is no) ])
dnl fi


dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************

dnl Do special configuration of Makefiles
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(Makefile.common)
AC_CONFIG_MAKEFILE(test/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile.tests)
AC_CONFIG_MAKEFILE(unittests/Makefile)

dnl This must be last
AC_OUTPUT
