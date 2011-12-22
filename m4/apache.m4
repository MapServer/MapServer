# macro that is used to parse a --with-apxs parameter
AC_DEFUN([APXS_CHECK],[
    AC_SUBST(APXS)
    
    AC_ARG_WITH(
        apxs,
        [  --with-apxs[=/path/to/apxs]     Apache 2 apxs tool location],
        ,
        [with_apxs="yes"]
    )
    
    if test "$with_apxs" = "yes"; then
        AC_PATH_PROG(APXS, apxs2)
        if test -z "$APXS"; then
            AC_PATH_PROG(APXS, apxs)
        fi
    elif test "$with_apxs" = "no"; then
        AC_MSG_ERROR(apxs is required and cannot be disabled)
    else
        AC_MSG_CHECKING(for apxs usability in $with_apxs)
        if test -x "$with_apxs"; then
            APXS=$with_apxs
            AC_MSG_RESULT(yes)
        else
            AC_MSG_ERROR($with_apxs not found or not executable)
        fi
    fi
    if test -z "$APXS"; then
        AC_MSG_ERROR(apxs utility not found. use --with-apxs to specify its location.)
    fi
    AC_SUBST(APXS)
    APACHE_SBINDIR=`$APXS -q SBINDIR`
    APACHE_BINDIR=`$APXS -q BINDIR`
    AC_SUBST(APACHE_SBINDIR)
    AC_SUBST(APACHE_BINDIR)
    AC_MSG_CHECKING([for apachectl utility])
    APACHECTL=
    if test -x "$APACHE_SBINDIR/apachectl" ; then
      APACHECTL="$APACHE_SBINDIR/apachectl" 
    else
      if test -x "$APACHE_SBINDIR/apache2ctl" ; then
        APACHECTL="$APACHE_SBINDIR/apache2ctl" 
      else
        AC_PATH_PROG(APACHECTL,apachectl)
        if test -z "$APACHECTL"; then
          AC_PATH_PROG(APACHECTL,apache2ctl)
        fi
      fi
    fi
    
    if test -z "$APACHECTL"; then
      AC_MSG_RESULT([Unable to find apachectl utility, you will not be able to restart
                   and install module with the created Makefile])
    else
      AC_MSG_RESULT([$APACHECTL])
    fi
    AC_SUBST(APACHECTL)
    AC_SUBST(APACHE_INC,-I`$APXS -q INCLUDEDIR`)
    AC_SUBST(APACHE_LIBS,`$APXS -q LIBS`)
    AC_SUBST(APACHE_LDFLAGS,`$APXS -q LDFLAGS`)
    SBINDIR=`$APXS -q SBINDIR`
    TARGET=`$APXS -q TARGET`
    HTTPD="$SBINDIR/$TARGET"
    AC_SUBST(HTTPD,"$HTTPD")
    
])
 
AC_DEFUN([APR_CHECK],[
  AC_SUBST(APRCONFIG)
  AC_ARG_WITH(apr_config,
    AC_HELP_STRING([--with-apr-config], [path to apr-config program]),
    ,
    [with_apr_config=yes]
  )
    if test "$with_apr_config" = "yes"; then
        AC_MSG_CHECKING(for apr-config in default locations)
        if test -n "$APXS"; then
            APXSFULL=`which "$APXS"`
            APXSDIR=`dirname "$APXSFULL"`
            if test -x "$APXSDIR/apr-config"; then
                APRCONFIG="$APXSDIR/apr-config"
            elif test -x "$APACHE_SBINDIR/apr-config"; then
                APRCONFIG="$APACHE_SBINDIR/apr-config"
            elif test -x "$APACHE_BINDIR/apr-config"; then
                APRCONFIG="$APACHE_BINDIR/apr-config"
            elif test -x "$APXSDIR/apr-1-config"; then
                APRCONFIG="$APXSDIR/apr-1-config"
            elif test -x "$APACHE_SBINDIR/apr-1-config"; then
                APRCONFIG="$APACHE_SBINDIR/apr-1-config"
            elif test -x "$APACHE_BINDIR/apr-1-config"; then
                APRCONFIG="$APACHE_BINDIR/apr-1-config"
            fi
        fi
        if test -z "$APRCONFIG"; then
            AC_PATH_PROG(APRCONFIG, apr-config)
        fi
        if test -z "$APRCONFIG"; then
            AC_PATH_PROG(APRCONFIG, apr-1-config)
        fi
        if test -n "$APRCONFIG"; then
            AC_MSG_RESULT([using $APRCONFIG, use --with-apr-config=/path/to/apr-(1-)config to modify])
        else
            AC_MSG_RESULT([not found])
        fi
    elif test "$with_apr_config" = "no"; then
        AC_MSG_ERROR(apr-config is required and cannot be disabled)
    else
        AC_MSG_CHECKING(for apr-config usability in $with_apr_config)
        if test -x "$with_apr_config"; then
            APRCONFIG=$with_apr_config
            AC_MSG_RESULT(yes)
        else
            AC_MSG_ERROR($with_apr_config not found or not executable)
        fi
    fi
    if test -z "$APRCONFIG"; then
        AC_MSG_ERROR(apr-config utility not found. use --with-apr-config to specify its location.)
    fi
    AC_SUBST(APRCONFIG)
    AC_SUBST(APR_CFLAGS,`$APRCONFIG --cppflags --cflags`) 
    AC_SUBST(APR_INC,`$APRCONFIG --includes`)
    AC_SUBST(APR_LIBS,`$APRCONFIG --link-libtool`)
])

