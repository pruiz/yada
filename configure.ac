
dnl ############################################################################
dnl ############################################################################

AC_PREREQ(2.52)
AC_INIT([yada], [1.0.0], [yada@devit.com], [yada])
AC_SUBST(LIB_VERSION, [7:0:3])
AC_REVISION($Revision: 191 $)

AC_PREFIX_DEFAULT([/usr/local])

dnl ############################################################################
dnl ## additional args

BUILD_EXTRA=

AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [enable debugging output]),
  [Debug="$enableval"
   CFLAGS="-g"], [Debug=no]
)
AC_SUBST(Debug)

STRIP_OPT=-s
AC_ARG_ENABLE(strip,
  AC_HELP_STRING([--disable-strip], [Disable calling strip(1) on install]),
  [if test "x$enableval" = "xno" ; then
    STRIP_OPT=
  fi]
)
AC_SUBST(STRIP_OPT)

AC_ARG_ENABLE(yadac,
  AC_HELP_STRING([--disable-yadac], [Disable yadac build and installation]),
  [ENABLE_YADAC="$enableval"], [ENABLE_YADAC=yes]
)
AC_SUBST(ENABLE_YADAC)


dnl ############################################################################
dnl ## check for requirements

dnl AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LEX

AC_CHECK_LIB([ltdl], [lt_dlinit], [WHICH_LIBLTDL="system" LIBLTDL="-lltdl"])

if test "x$WHICH_LIBLTDL" = "x"; then
  AC_LIBLTDL_CONVENIENCE
  AC_CONFIG_SUBDIRS(libltdl)
  BUILD_EXTRA="$BUILD_EXTRA libltdl"
  WHICH_LIBLTDL="bundled"
fi

AC_SUBST(LTDLINCL)
AC_SUBST(LIBLTDL)

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_HEADER_STDC
AC_HEADER_STAT
AC_CHECK_HEADERS([string.h unistd.h])

AC_DEFINE(_GNU_SOURCE, 1, [define for GNU extensions])

AC_CHECK_FUNCS([strndup])

dnl ############################################################################
dnl ## check for available modules

CHECK_MYSQL
CHECK_ORACLE
CHECK_OCI_VERSION
CHECK_PGSQL
CHECK_SQLITE3

AC_SUBST(YADA_BUILD_MODULES)
AC_DEFINE_UNQUOTED(YADA_BUILD_MODULES, "$YADA_BUILD_MODULES",
          [modules yada was compiled with])

dnl ############################################################################
dnl ## additional processing

if test "$Debug" = "yes"; then
  AC_DEFINE(DEBUGMODE, 1, [Define if you want debug mode on])
fi

dnl ############################################################################
dnl ## generate files


if test "$ENABLE_YADAC" = "yes"; then
  BUILD_EXTRA="$BUILD_EXTRA yadac"
fi

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(BUILD_EXTRA)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(docs/doxygen.conf)
AC_CONFIG_FILES(docs/version.texi)
AC_CONFIG_FILES(docs/Makefile)
AC_CONFIG_FILES(test/Makefile)
AC_CONFIG_FILES(yadac/Makefile)
AC_CONFIG_FILES(contrib/yada.spec)
AC_CONFIG_FILES([contrib/yada-$PACKAGE_VERSION.ebuild:contrib/yada.ebuild])

AC_CONFIG_FILES($CONFIG_EXTRA)

AC_OUTPUT

dnl ############################################################################
dnl ## print summary

echo
echo '############################################################'
echo 'Yada configuration complete.'
echo
echo "prefix:            $prefix"
echo "yadac:             $ENABLE_YADAC"
echo "libltdl:           $WHICH_LIBLTDL"
echo

echo "MySQL:             $check_mysql"
if test "$check_mysql" = yes; then
  echo "  headers from:    $check_mysql_incdir"
  echo "  libs from:       $check_mysql_libdir"
fi

echo "Oracle:            $check_oracle"
if test "$check_oracle" = yes; then
  echo "  headers from:    $check_oracle_incdir"
  echo "  libs from:       $check_oracle_libdir"
fi

echo "PostgreSQL:        $check_pgsql"
if test "$check_pgsql" = yes; then
  echo "  headers from:    $check_pgsql_incdir"
  echo "  libs from:       $check_pgsql_libdir"
fi

echo "SQLite3:           $check_sqlite3"
if test "$check_sqlite3" = yes; then
  echo "  headers from:    $check_sqlite3_incdir"
  echo "  libs from:       $check_sqlite3_libdir"
fi

echo '############################################################'
echo

dnl ############################################################################
dnl ############################################################################

