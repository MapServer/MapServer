
dnl
dnl AC_EXPAND_PATH(path, variable)
dnl
dnl expands path to an absolute path and assigns it to variable
dnl
AC_DEFUN(AC_EXPAND_PATH,[
  if test -z "$1" || echo "$1" | grep '^/' >/dev/null ; then
    $2="$1"
  else
    $2="`pwd`/$1"
  fi
])


dnl
dnl AC_PARSE_WITH_LIB_STATIC(with_param, out_libpath, out_isstatic)
dnl
dnl parse a --with-lib=[static,]/path/to/lib string, and set out_libpath 
dnl with the library path, and out_isstatic=yes if "static," was there.
dnl
AC_DEFUN(AC_PARSE_WITH_LIB_STATIC,[
  if echo "$1" | grep '^static,' >/dev/null ; then
    $2=`echo "$1" | sed "s/^static,//"`
    $3=yes
  else
    $2=$1
    $3=no
  fi
])


AC_DEFUN(AC_COMPILER_PIC,
[
	echo 'void f(){}' > conftest.c
	if test -z "`${CC} -fPIC -c conftest.c 2>&1`"; then
	  C_PIC=-fPIC
	else
	  C_PIC=
	fi
	rm -f conftest*

	AC_SUBST(C_PIC,$C_PIC)
])


dnl
dnl check for -R, etc. switch
dnl 
dnl This was borrowed and adapted from the PHP configure.in script
dnl
AC_DEFUN(AC_RUNPATH_SWITCH,
[
  AC_MSG_CHECKING(if compiler supports -R)
  AC_CACHE_VAL(php_cv_cc_dashr,[
	SAVE_LIBS="${LIBS}"
	LIBS="-R /usr/lib ${LIBS}"
	AC_TRY_LINK([], [], php_cv_cc_dashr=yes, php_cv_cc_dashr=no)
	LIBS="${SAVE_LIBS}"])
  AC_MSG_RESULT($php_cv_cc_dashr)
  if test $php_cv_cc_dashr = "yes"; then
	ld_runpath_switch="-R"
	apxs_runpath_switch="-Wl,-R'"
  fi
  if test -z "$ld_runpath_switch" ; then
	AC_MSG_CHECKING([if compiler supports -Wl,-rpath,])
	AC_CACHE_VAL(php_cv_cc_rpath,[
		SAVE_LIBS="${LIBS}"
		LIBS="-Wl,-rpath,/usr/lib ${LIBS}"
		AC_TRY_LINK([], [], php_cv_cc_rpath=yes, php_cv_cc_rpath=no)
		LIBS="${SAVE_LIBS}"])
	AC_MSG_RESULT($php_cv_cc_rpath)
	if test $php_cv_cc_rpath = "yes"; then
		ld_runpath_switch="-Wl,-rpath,"
		apxs_runpath_switch="-Wl,'-rpath "
	fi
  fi
  if test -z "$ld_runpath_switch" ; then
	AC_MSG_CHECKING([if compiler supports -Wl,-R])
	AC_CACHE_VAL(php_cv_cc_dashwlr,[
		SAVE_LIBS="${LIBS}"
		LIBS="-Wl,-R/usr/lib ${LIBS}"
		AC_TRY_LINK([], [], php_cv_cc_dashwlr=yes, php_cv_cc_dashwlr=no)
		LIBS="${SAVE_LIBS}"])
	AC_MSG_RESULT($php_cv_cc_dashwlr)
	if test $php_cv_cc_rpath = "yes"; then
		ld_runpath_switch="-Wl,-R"
		apxs_runpath_switch="-Wl,-R'"
	fi
  fi
  if test -z "$ld_runpath_switch" ; then
	dnl something innocuous
	ld_runpath_switch="-L"
	apxs_runpath_switch="-L'"
  fi
  
])


dnl
dnl AC_PHP_ONCE(namespace, variable, code)
dnl
dnl execute code, if variable is not set in namespace
dnl
AC_DEFUN(AC_PHP_ONCE,[
  unique=`echo $ac_n "$2$ac_c" | tr -c -d a-zA-Z0-9`
  cmd="echo $ac_n \"\$$1$unique$ac_c\""
  if test -n "$unique" && test "`eval $cmd`" = "" ; then
    eval "$1$unique=set"
    $3
  fi
])

dnl
dnl AC_ADD_RUNPATH(path)
dnl
dnl add a library to linkpath/runpath stored in RPATH 
dnl works together with AC_RUNPATH_SWITCH()
dnl
AC_DEFUN(AC_ADD_RUNPATH,[
  if test "$1" != "/usr/lib"; then
    AC_EXPAND_PATH($1, ai_p)
    AC_PHP_ONCE(LIBPATH, $ai_p, [
      EXTRA_LIBS="$EXTRA_LIBS -L$ai_p"
dnl      if test -n "$APXS" ; then
dnl        RPATHS="$RPATHS ${apxs_runpath_switch}$ai_p'"
dnl      else
        RPATHS="$RPATHS ${ld_runpath_switch}$ai_p"
dnl      fi
    ])
  fi
])


dnl
dnl Try to find something to link shared libraries with.  Use "c++ -shared"
dnl in preference to "ld -shared" because it will link in required c++
dnl run time support for us. 
dnl
AC_DEFUN(AC_LD_SHARED,
[
  echo 'void g(); int main(){ g(); return 0; }' > conftest1.c

  echo '#include <stdio.h>' > conftest2.c
  echo 'void g(); void g(){printf("");}' >> conftest2.c
  ${CC} ${C_PIC} -c conftest2.c

  SO_EXT="so"
  export SO_EXT
  LD_SHARED="/bin/true"
  if test ! -z "`uname -a | grep IRIX`" ; then
    IRIX_ALL=-all
  else
    IRIX_ALL=
  fi

  SHARED_FLAG="-shared"

  dnl MacOSX
  if test -n "`uname -a | grep Darwin`" ; then
    SHARED_FLAG="-bundle -flat_namespace -undefined suppress"
    LD_SHARED="${CXX} $SHARED_FLAG"
  fi

  if test -z "`${CXX} $SHARED_FLAG $IRIX_ALL conftest2.o -o libconftest.so 2>&1|grep -v WARNING`" ; then
    if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
      LD_LIBRARY_PATH_OLD="$LD_LIBRARY_PATH"
      if test -z "$LD_LIBRARY_PATH" ; then
        LD_LIBRARY_PATH="`pwd`"
      else
        LD_LIBRARY_PATH="`pwd`:$LD_LIBRARY_PATH"
      fi
      export LD_LIBRARY_PATH
      if test -z "`./conftest1 2>&1`" ; then
        echo "checking for ${CXX} $SHARED_FLAG ... yes"
        LD_SHARED="${CXX} $SHARED_FLAG $IRIX_ALL"
      else
        echo "checking for ${CXX} $SHARED_FLAG ... no(3)"
      fi
      LD_LIBRARY_PATH="$LD_LIBRARY_PATH_OLD"
    else
      echo "checking for ${CXX} $SHARED_FLAG ... no(2)"
    fi
  else
    echo "checking for ${CXX} $SHARED_FLAG ... no(1)"
  fi

  if test "$LD_SHARED" = "/bin/true" \
          -a -z "`ld $SHARED_FLAG conftest2.o -o libconftest.so 2>&1`" ; then
    if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
      LD_LIBRARY_PATH_OLD="$LD_LIBRARY_PATH"
      if test -z "$LD_LIBRARY_PATH" ; then
        LD_LIBRARY_PATH="`pwd`"
      else
        LD_LIBRARY_PATH="`pwd`:$LD_LIBRARY_PATH"
      fi
      export LD_LIBRARY_PATH
      if test -z "`./conftest1 2>&1`" ; then
        echo "checking for ld $SHARED_FLAG ... yes"
        LD_SHARED="ld $SHARED_FLAG"
      fi
      LD_LIBRARY_PATH="$LD_LIBRARY_PATH_OLD"
    fi
  fi

  if test "$LD_SHARED" = "/bin/true" \
          -a -z "`${CC} -dynamiclib conftest2.o -o libconftest.so 2>&1`" ; then
    if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
      DYLD_LIBRARY_PATH_OLD="$DYLD_LIBRARY_PATH"
      if test -z "$DYLD_LIBRARY_PATH" ; then
        DYLD_LIBRARY_PATH="`pwd`"
      else
        DYLD_LIBRARY_PATH="`pwd`:$DYLD_LIBRARY_PATH"
      fi
      export DYLD_LIBRARY_PATH
      if test -z "`./conftest1 2>&1`" ; then
        echo "checking for ${CC} -dynamiclib ... yes"
        LD_SHARED="${CC} -dynamiclib"
	SO_EXT=dylib
      fi
      DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH_OLD"
    fi
  fi

  if test "$LD_SHARED" = "/bin/true" ; then
    echo "checking for ld $SHARED_FLAG ... no"
    if test ! -x /bin/true ; then
      LD_SHARED=/usr/bin/true
    fi
  fi
  rm -f conftest* libconftest* 

  echo "        using '${CXX} $SHARED_FLAG' for dynamic linking"

  AC_SUBST(LD_SHARED,$LD_SHARED)
  AC_SUBST(SO_EXT,$SO_EXT)
])


dnl
dnl The following macro is actually based on the "setup" script that
dnl was included in the php-3.0.14 dl directory and will look at the 
dnl Perl compile flags to figure the mechanism to build a shared lib
dnl on this system.
dnl This is the preferred macro for the PHP module, but if perl is not
dnl available then we can always fallback on AC_LD_SHARED above.
dnl

AC_DEFUN(AC_LD_SHARED_FROM_PERL,
[
#
# The following is a massive hack.  It tries to steal the
# mechanism for build a dynamic library from Perl's -V output
# If this script fails on this machine, try running 'perl -V'
# manually and pick out the setting for:
#   
#    cc, optimize, ccflags, ld, cccdlflags and lddlflags
#
# Replace the below definitions with the output you see.
#

if test ! -r "perl.out"; then
	perl -V > perl.out
fi

# if the greps and cuts don't do the job, set these manually
PERL_CC="`grep ' cc=' perl.out | cut -d, -f1 | cut -d\' -f2 | grep -v undef`"
PERL_OPT="`grep ' optimize=' perl.out | cut -d, -f2 | cut -d\' -f2 | grep -v undef`"
PERL_CCFLAGS="`grep ' ccflags' perl.out | cut -d, -f1 | grep cflags | cut -d\' -f2 | grep -v undef` `grep ' ccflags' perl.out | cut -d, -f2 | grep ccflags | cut -d\' -f2 | grep -v undef`"
PERL_LD="`grep ' ld=' perl.out | cut -d, -f1 | cut -d\' -f2 | grep -v undef`"
PERL_LFLAGS="`grep ' cccdlflags=' perl.out | cut -d, -f1 | cut -d\' -f2 | grep -v undef`"
PERL_CCDLFLAGS="`grep ' ccdlflags=' perl.out | cut -d, -f4 | cut -d\' -f2 | sed 's, ,,' | grep -v undef`"
PERL_LDDLFLAGS=`grep ' lddlflags' perl.out | cut -d, -f2 | cut -d\' -f2 | grep -v undef`
#--------

#if test -n "$PERL_CCDLFLAGS" ; then
#	echo "-------------------------"
#	echo "----- IMPORTANT !!! -----"
#	echo "-------------------------"
#	echo "To use PHP extensions on your OS, you will need to recompile "
#	echo "PHP.                                                         "
#	echo "You need to edit the Makefile in the php3 directory and add  "
#	echo "$PERL_CCDLFLAGS to the start of the LDFLAGS line at the top  "
#	echo "of the Makefile.  Then type: 'make clean; make'              "
#	echo "You can still go ahead and build the extensions now by typing"
#	echo "'make' in this directory.  They just won't work correctly    "
#	echo "until you recompile your PHP.                                "
#        echo "If you are compiling php as a module, you should also add    "
#        echo "$PERL_CCDLFLAGS to the start of the EXTRA_LDFLAGS in Apache  "
#        echo "Configuration file.  Note that if you are using the APACI    "
#        echo "build mechanism you should make this change in the           "
#        echo "Configuration.tmpl file instead.                             "
#	echo "-------------------------"
#	echo "-------------------------"
#	echo "-------------------------"
#fi

PERL_CC="$PERL_CC $PERL_OPT $PERL_CCFLAGS -I. -I.. $PERL_LFLAGS"
PERL_LD="$PERL_LD $PERL_LDDLFLAGS $PERL_CCDLFLAGS"

rm -f perl.out

])

