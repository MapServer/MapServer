
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



AC_DEFUN(AC_COMPILER_WFLAGS,
[
	# Remove -g from compile flags, we will add via CFG variable if
	# we need it.
	CXXFLAGS=`echo "$CXXFLAGS " | sed "s/-g //"`
	CFLAGS=`echo "$CFLAGS " | sed "s/-g //"`

	# check for GNU compiler, and use -Wall
	if test "$GCC" = "yes"; then
		C_WFLAGS="-Wall"
		CFLAGS="$CFLAGS -Wall"
		AC_DEFINE(USE_GNUCC)
	fi
	if test "$GXX" = "yes"; then
		CXX_WFLAGS="-Wall"
		CXXFLAGS="$CXXFLAGS -Wall"
		AC_DEFINE(USE_GNUCC)
	fi
	AC_SUBST(CXX_WFLAGS,$CXX_WFLAGS)
	AC_SUBST(C_WFLAGS,$C_WFLAGS)
])

AC_DEFUN(AC_COMPILER_WFLAGS_DEBUG,
[
	# -g should already be in compile flags.
	# check for GNU compiler, and add -Wall
	if test "$GCC" = "yes"; then
		C_WFLAGS="-Wall"
		CFLAGS="$CFLAGS -Wall"
		AC_DEFINE(USE_GNUCC)
	fi
	if test "$GXX" = "yes"; then
		CXX_WFLAGS="-Wall"
		CXXFLAGS="$CXXFLAGS -Wall"
		AC_DEFINE(USE_GNUCC)
	fi
	AC_SUBST(CXX_WFLAGS,$CXX_WFLAGS)
	AC_SUBST(C_WFLAGS,$C_WFLAGS)
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
dnl Try to find something to link shared libraries with.  Try "gcc -shared"
dnl first and then "ld -shared".
dnl For some reason, on SunOS, gcc -shared appears to work but fails with
dnl the PHP module... so try to use "ld -G" instead on those systems.
dnl
AC_DEFUN(AC_LD_SHARED,
[
  echo 'void g(); int main(){ g(); return 0; }' > conftest1.c

  echo 'void g(); void g(){}' > conftest2.c
  ${CC} ${C_PIC} -c conftest2.c

  LD_SHARED="/bin/true"

  if test "`uname -s`" = "SunOS" \
          -a -z "`ld -G conftest2.o -o libconftest.so 2>&1`" ; then
    if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
      LD_LIBRARY_PATH_OLD="$LD_LIBRARY_PATH"
      LD_LIBRARY_PATH="`pwd`"
      export LD_LIBRARY_PATH
      if test -z "`./conftest1 2>&1`" ; then
        echo "checking for ld -G ... yes"
        LD_SHARED="ld -G"
      fi
      LD_LIBRARY_PATH="$LD_LIBRARY_PATH_OLD"
    fi
  fi

  if test "$LD_SHARED" = "/bin/true" ; then
    if test -z "`${CC} -shared conftest2.o -o libconftest.so 2>&1`" ; then
      if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
        LD_LIBRARY_PATH_OLD="$LD_LIBRARY_PATH"
        LD_LIBRARY_PATH="`pwd`"
        export LD_LIBRARY_PATH
        if test -z "`./conftest1 2>&1`" ; then
          echo "checking for ${CC} -shared ... yes"
          LD_SHARED="${CC} -shared"
        else
          echo "checking for ${CC} -shared ... no(3)"
        fi
        LD_LIBRARY_PATH="$LD_LIBRARY_PATH_OLD"
      else
        echo "checking for ${CC} -shared ... no(2)"
      fi
    else
      echo "checking for ${CC} -shared ... no(1)"
    fi
  fi

  if test "$LD_SHARED" = "/bin/true" \
          -a -z "`ld -shared conftest2.o -o libconftest.so 2>&1`" ; then
    if test -z "`${CC} conftest1.c libconftest.so -o conftest1 2>&1`"; then
      LD_LIBRARY_PATH_OLD="$LD_LIBRARY_PATH"
      LD_LIBRARY_PATH="`pwd`"
      export LD_LIBRARY_PATH
      if test -z "`./conftest1 2>&1`" ; then
        echo "checking for ld -shared ... yes"
        LD_SHARED="ld -shared"
      fi
      LD_LIBRARY_PATH="$LD_LIBRARY_PATH_OLD"
    fi
  fi

  if test "$LD_SHARED" = "/bin/true" ; then
    echo "checking for ld -shared ... no"
  fi
  rm -f conftest* libconftest* 

  AC_SUBST(LD_SHARED,$LD_SHARED)
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
PERL_CC="`grep cc= perl.out | cut -d, -f1 | cut -d\' -f2`"
PERL_OPT="`grep optimize= perl.out | cut -d, -f2 | cut -d\' -f2`"
PERL_CCFLAGS="`grep ccflags perl.out | cut -d, -f1 | cut -d\' -f2`"
PERL_LD="`grep ld= perl.out | cut -d, -f1 | cut -d\' -f2`"
PERL_LFLAGS="`grep cccdlflags= perl.out | cut -d, -f1 | cut -d\' -f2`"
PERL_CCDLFLAGS="`grep ccdlflags= perl.out | cut -d, -f4 | cut -d\' -f2 | sed 's, ,,'`"
PERL_LDDLFLAGS=`grep lddlflags perl.out | cut -d, -f2 | cut -d\' -f2`
#--------

if test -n "$PERL_CCDLFLAGS" ; then
	echo "-------------------------"
	echo "----- IMPORTANT !!! -----"
	echo "-------------------------"
	echo "To use PHP extensions on your OS, you will need to recompile "
	echo "PHP.                                                         "
	echo "You need to edit the Makefile in the php3 directory and add  "
	echo "$PERL_CCDLFLAGS to the start of the LDFLAGS line at the top  "
	echo "of the Makefile.  Then type: 'make clean; make'              "
	echo "You can still go ahead and build the extensions now by typing"
	echo "'make' in this directory.  They just won't work correctly    "
	echo "until you recompile your PHP.                                "
        echo "If you are compiling php as a module, you should also add    "
        echo "$PERL_CCDLFLAGS to the start of the EXTRA_LDFLAGS in Apache  "
        echo "Configuration file.  Note that if you are using the APACI    "
        echo "build mechanism you should make this change in the           "
        echo "Configuration.tmpl file instead.                             "
	echo "-------------------------"
	echo "-------------------------"
	echo "-------------------------"
fi

PERL_CC="$PERL_CC $PERL_OPT $PERL_CCFLAGS -I. -I.. $PERL_LFLAGS"
PERL_LD="$PERL_LD $PERL_LDDLFLAGS $PERL_CCDLFLAGS"

rm -f perl.out

])

