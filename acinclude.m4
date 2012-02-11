
################################################################################
################################################################################

# acinclude.m4
# yada specific macros
#
# $Id: acinclude.m4 170 2007-06-11 15:00:51Z grizz $

################################################################################
# find a file from a list of directories

AC_DEFUN([FIND_FILE],
[
$3=""
for i in $2; do
  for j in $1; do
    if test -r "$i/$j"; then
      $3=$i
      break 2
    fi
  done
done
])

################################################################################
## add to build modules

AC_DEFUN([YADA_ADD_MODULE],
[
  if test -z "$YADA_BUILD_MODULES"; then
    YADA_BUILD_MODULES="$1"
  else
    YADA_BUILD_MODULES="$YADA_BUILD_MODULES $1"
  fi
])

################################################################################
# mysql

AC_DEFUN([CHECK_MYSQL],
[
  AC_CACHE_CHECK([MySQL support], check_mysql, [

    AC_ARG_WITH(mysql,
      AC_HELP_STRING([--with-mysql@<:@=DIR@:>@],
      [Include MySQL support from DIR        @<:@default=auto@:>@]),
      [check_mysql="$withval"], [check_mysql="auto"])
    AC_ARG_WITH(mysql-incdir,
      AC_HELP_STRING([--with-mysql-incdir=DIR],
      [Use MySQL include files from DIR   @<:@default=auto@:>@]),
      [check_mysql_incdir="$withval"], [check_mysql_incdir="auto"])
    AC_ARG_WITH(mysql-libdir,
      AC_HELP_STRING([--with-mysql-libdir=DIR],
      [Use MySQL libraries from DIR  @<:@default=auto@:>@]),
      [check_mysql_libdir="$withval"], [check_mysql_libdir="auto"])

    dnl find and test if it hasn't been disabled
    if test "$check_mysql" != "no"; then

      dnl note if auto, and then set to yes for tests
      if test "$check_mysql" = "auto"; then
        mysql_auto=yes
        check_mysql=yes
      fi

      dnl look in common places
      if test "$check_mysql" = "yes"; then
        mysql_incdirs="/usr/include /usr/local/include /usr/include/mysql \
                       /usr/local/include/mysql /usr/local/mysql/include \
                       /usr/local/mysql/include/mysql /opt/mysql/include/mysql \
                       /sw/include/mysql"
        mysql_libdirs="/usr/lib /usr/lib/mysql /usr/local/lib \
                       /usr/local/lib/mysql /usr/local/mysql/lib \
                       /usr/local/mysql/lib/mysql /opt/mysql/lib \
                       /opt/mysql/lib/mysql /sw/lib/mysql"

      dnl if a directory was given to --with-mysql, only look there
      else
        mysql_incdirs="$check_mysql/include $check_mysql/include/mysql"
        mysql_libdirs="$check_mysql/lib $check_mysql/lib/mysql"
        check_mysql=yes
      fi

      dnl override if either dir was specified and only look there
      if test ! "$check_mysql_incdir" = "auto"; then
        mysql_incdirs="$check_mysql_incdir $check_mysql_incdir/mysql"
      fi
      if test ! "$check_mysql_libdir" = "auto"; then
        mysql_libdirs="$check_mysql_libdir $check_mysql_libdir/mysql"
      fi

      dnl check for header existance
      FIND_FILE(mysql.h, $mysql_incdirs, check_mysql_incdir)
      if test -z "$check_mysql_incdir"; then
        check_mysql="no"
        if test ! "$mysql_auto" = "yes"; then
          AC_MSG_ERROR([MySQL headers not found])
        fi
      fi

      dnl check for lib existance
      FIND_FILE([libmysqlclient.so libmysqlclient.a], $mysql_libdirs,
                check_mysql_libdir)
      if test -z "$check_mysql_libdir"; then
        check_mysql="no"
        if test ! "$mysql_auto" = "yes"; then
          AC_MSG_ERROR([MySQL headers not found])
        fi
      fi

      dnl try using it to make sure it works
      if test "$check_mysql" = "yes"; then
        ORIG_LDFLAGS="$LDFLAGS"
        LDFLAGS="-I$check_mysql_incdir -L$check_mysql_libdir \
                 -Wl,-R$check_mysql_libdir -lmysqlclient -lm"
        AC_RUN_IFELSE( [
          AC_LANG_PROGRAM( [[ 
            #include <mysql.h>
          ]], [[
            MYSQL mysql;
            mysql_init(&mysql);
            mysql_real_connect(&mysql, 0, 0, 0, 0, 0, 0, 0);
            exit(0);
          ]]
        )],
        [
          check_mysql=yes
          LDFLAGS="$ORIG_LDFLAGS"
        ],
        [AC_MSG_FAILURE([compiling and/or running test MySQL program])])
      fi
    fi
  ])

  if test "$check_mysql" = "yes"; then
    MYSQL_LIBS="-lmysqlclient -lm"
    MYSQL_INCLUDE="-I$check_mysql_incdir"
    MYSQL_LDFLAGS="-L$check_mysql_libdir -R$check_mysql_libdir"
    YADA_ADD_MODULE("mysql")
  
    AC_SUBST(MYSQL_LIBS)
    AC_SUBST(MYSQL_INCLUDE)
    AC_SUBST(MYSQL_LDFLAGS)
  else
    MYSQL_LIBS=""
    MYSQL_LDFLAGS=""
    MYSQL_INCLUDE=""
  fi
])


################################################################################
# oracle

AC_DEFUN([CHECK_ORACLE],
[
  AC_CACHE_CHECK([Oracle support], check_oracle, [

    AC_ARG_WITH(oracle,
      AC_HELP_STRING([--with-oracle@<:@=DIR@:>@],
      [Include Oracle support from DIR        @<:@default=auto@:>@]),
      [check_oracle="$withval"], [check_oracle="auto"])
    AC_ARG_WITH(oracle-incdir,
      AC_HELP_STRING([--with-oracle-incdir=DIR],
      [Use Oracle include files from DIR   @<:@default=auto@:>@]),
      [check_oracle_incdir="$withval"], [check_oracle_incdir="auto"])
    AC_ARG_WITH(oracle-libdir,
      AC_HELP_STRING([--with-oracle-libdir=DIR],
      [Use Oracle libraries from DIR  @<:@default=auto@:>@]),
      [check_oracle_libdir="$withval"], [check_oracle_libdir="auto"])

    dnl find and test if it hasn't been disabled
    if test "$check_oracle" != "no"; then

      dnl note if auto, and then set to yes for tests
      if test "$check_oracle" = "auto"; then
        oracle_auto=yes
        check_oracle=yes
      fi

      dnl look in $ORACLE_HOME
      if test "$check_oracle" = "yes"; then
        oracle_incdirs="$ORACLE_HOME/rdbms/demo $ORACLE_HOME/rdbms/public \
                        $ORACLE_HOME/include"
        oracle_libdirs="$ORACLE_HOME/lib32 $ORACLE_HOME/lib"

      dnl if a directory was given to --with-oracle, only look there
      else
        oracle_incdirs="$check_oracle/rdbms/demo $check_oracle/rdbms/public"
        oracle_libdirs="$check_oracle/lib32 $check_oracle/lib"
        check_oracle=yes
      fi

      dnl override if either dir was specified and only look there
      if test ! "$check_oracle_incdir" = "auto"; then
        oracle_incdirs="$check_oracle_incdir $check_oracle_incdir/oracle"
      fi
      if test ! "$check_oracle_libdir" = "auto"; then
        oracle_libdirs="$check_oracle_libdir $check_oracle_libdir/oracle"
      fi

      oracle_cppflags=''
      dnl check for header existance
      FIND_FILE(oci.h, $oracle_incdirs, check_oracle_incdir)
      if test -z "$check_oracle_incdir"; then
        check_oracle="no"
        if test ! "$oracle_auto" = "yes"; then
          AC_MSG_ERROR([oci.h not found])
        fi
      else
        oracle_cppflags="-I$check_oracle_incdir"
      fi

      dnl check for header existance
      FIND_FILE(nzt.h, $oracle_incdirs, check_oracle_incdir)
      if test -z "$check_oracle_incdir"; then
        check_oracle="no"
        if test ! "$oracle_auto" = "yes"; then
          AC_MSG_ERROR([nzt.h not found])
        fi
      else
        oracle_cppflags="$oracle_cppflags -I$check_oracle_incdir"
      fi

      dnl check for lib existance
      FIND_FILE([libclntsh.so], $oracle_libdirs,
                check_oracle_libdir)
      if test -z "$check_oracle_libdir"; then
        check_oracle="no"
        if test ! "$oracle_auto" = "yes"; then
          AC_MSG_ERROR([Oracle libraries not found])
        fi
      fi

      dnl try using it to make sure it works
      if test "$check_oracle" = "yes"; then
        ORIG_LDFLAGS="$LDFLAGS"
        LDFLAGS="$oracle_cppflags -L$check_oracle_libdir \
                 -Wl,-R$check_oracle_libdir -lclntsh"
        AC_RUN_IFELSE( [
          AC_LANG_PROGRAM( [[ 
            #include <oci.h>
          ]], [[
            OCIEnv *env;
            OCIEnvCreate(&env, OCI_DEFAULT, 0, 0, 0, 0, 0, 0);
            exit(0);
          ]]
        )],
        [
          check_oracle=yes
          LDFLAGS="$ORIG_LDFLAGS"
        ],
        [AC_MSG_FAILURE([compiling and/or running test Oracle program])])
      fi
    fi
  ])

  if test "$check_oracle" = "yes"; then
    ORACLE_LIBS="-lclntsh -lm"
    ORACLE_INCLUDE="$oracle_cppflags"
    ORACLE_LDFLAGS="-L$check_oracle_libdir -R$check_oracle_libdir"
    YADA_ADD_MODULE("oracle")
  
    AC_SUBST(ORACLE_LIBS)
    AC_SUBST(ORACLE_INCLUDE)
    AC_SUBST(ORACLE_LDFLAGS)
  else
    ORACLE_LIBS=""
    ORACLE_LDFLAGS=""
    ORACLE_INCLUDE=""
  fi
])

dnl test for older versions of OCI
AC_DEFUN([CHECK_OCI_VERSION],
[
  AC_CACHE_CHECK([for outdated OCI version], obsolete_oci, [
    AC_COMPILE_IFELSE( [
      AC_LANG_PROGRAM( [[ 
        #include <oci.h>
      ]], [[
        OCISvcCtx *ctx;
        OCIStmt *stmt;
        OCIStmtPrepare2(&ctx, &stmt, NULL, NULL, 0, NULL, 0, 0, 0);
        exit(0);
      ]]
    )], [
      obsolete_oci=no
    ], [
      obsolete_oci=yes
    ])
  ])

  if test "$obsolete_oci" = yes; then
    ORACLE_CFLAGS='-DOBSOLETE_OCI'
    AC_SUBST(ORACLE_CFLAGS)
  fi
])

################################################################################
# postgresql

AC_DEFUN([CHECK_PGSQL],
[
  AC_CACHE_CHECK([PgSQL support], check_pgsql, [

    AC_ARG_WITH(pgsql,
      AC_HELP_STRING([--with-pgsql@<:@=DIR@:>@],
      [Include PgSQL support from DIR        @<:@default=auto@:>@]),
      [check_pgsql="$withval"], [check_pgsql="auto"])
    AC_ARG_WITH(pgsql-incdir,
      AC_HELP_STRING([--with-pgsql-incdir=DIR],
      [Use PgSQL include files from DIR   @<:@default=auto@:>@]),
      [check_pgsql_incdir="$withval"], [check_pgsql_incdir="auto"])
    AC_ARG_WITH(pgsql-libdir,
      AC_HELP_STRING([--with-pgsql-libdir=DIR],
      [Use PgSQL libraries from DIR  @<:@default=auto@:>@]),
      [check_pgsql_libdir="$withval"], [check_pgsql_libdir="auto"])

    dnl find and test if it hasn't been disabled
    if test "$check_pgsql" != "no"; then

      dnl note if auto, and then set to yes for tests
      if test "$check_pgsql" = "auto"; then
        pgsql_auto=yes
        check_pgsql=yes
      fi

      dnl look in common places
      if test "$check_pgsql" = "yes"; then
        pgsql_incdirs="/usr/include /usr/local/include /usr/include/postgresql \
                       /sw/include /usr/local/pgsql/include /opt/pgsql/include"
        pgsql_libdirs="/usr/lib /usr/local/lib /sw/lib \
                       /lib /usr/local/pgsql/lib /opt/pgsql/lib"

      dnl if a directory was given to --with-pgsql, only look there
      else
        pgsql_incdirs="$check_pgsql/include"
        pgsql_libdirs="$check_pgsql/lib"
        check_pgsql=yes
      fi

      dnl override if either dir was specified and only look there
      if test ! "$check_pgsql_incdir" = "auto"; then
        pgsql_incdirs="$check_pgsql_incdir $check_pgsql_incdir/pgsql"
      fi
      if test ! "$check_pgsql_libdir" = "auto"; then
        pgsql_libdirs="$check_pgsql_libdir $check_pgsql_libdir/pgsql"
      fi

      dnl check for header existance
      FIND_FILE(libpq-fe.h, $pgsql_incdirs, check_pgsql_incdir)
      if test -z "$check_pgsql_incdir"; then
        check_pgsql="no"
        if test ! "$pgsql_auto" = "yes"; then
          AC_MSG_ERROR([PgSQL headers not found])
        fi
      fi

      dnl check for lib existance
      FIND_FILE([libpq.so libpq.a], $pgsql_libdirs,
                check_pgsql_libdir)
      if test -z "$check_pgsql_libdir"; then
        check_pgsql="no"
        if test ! "$pgsql_auto" = "yes"; then
          AC_MSG_ERROR([PgSQL libraries not found])
        fi
      fi

      dnl try using it to make sure it works
      if test "$check_pgsql" = "yes"; then
        ORIG_LDFLAGS="$LDFLAGS"
        LDFLAGS="-I$check_pgsql_incdir -L$check_pgsql_libdir \
                 -Wl,-R$check_pgsql_libdir -lpq"
        AC_RUN_IFELSE( [
          AC_LANG_PROGRAM( [[ 
            #include <libpq-fe.h>
          ]], [[
            PQconnectdb("");
            exit(0);
          ]]
        )],
        [
          check_pgsql=yes
          LDFLAGS="$ORIG_LDFLAGS"
        ],
        [AC_MSG_FAILURE([compiling and/or running test PgSQL program])])
      fi
    fi
  ])

  if test "$check_pgsql" = "yes"; then
    PGSQL_LIBS="-lpq"
    PGSQL_INCLUDE="-I$check_pgsql_incdir"
    PGSQL_LDFLAGS="-L$check_pgsql_libdir -R$check_pgsql_libdir"
    YADA_ADD_MODULE("pgsql")
  
    AC_SUBST(PGSQL_LIBS)
    AC_SUBST(PGSQL_INCLUDE)
    AC_SUBST(PGSQL_LDFLAGS)
  else
    PGSQL_LIBS=""
    PGSQL_LDFLAGS=""
    PGSQL_INCLUDE=""
  fi
])

################################################################################
# sqlite3

AC_DEFUN([CHECK_SQLITE3],
[
  AC_CACHE_CHECK([SQLite3 support], check_sqlite3, [

    AC_ARG_WITH(sqlite3,
      AC_HELP_STRING([--with-sqlite3@<:@=DIR@:>@],
      [Include SQLite3 support from DIR        @<:@default=auto@:>@]),
      [check_sqlite3="$withval"], [check_sqlite3="auto"])
    AC_ARG_WITH(sqlite3-incdir,
      AC_HELP_STRING([--with-sqlite3-incdir=DIR],
      [Use SQLite3 include files from DIR   @<:@default=auto@:>@]),
      [check_sqlite3_incdir="$withval"], [check_sqlite3_incdir="auto"])
    AC_ARG_WITH(sqlite3-libdir,
      AC_HELP_STRING([--with-sqlite3-libdir=DIR],
      [Use SQLite3 libraries from DIR  @<:@default=auto@:>@]),
      [check_sqlite3_libdir="$withval"], [check_sqlite3_libdir="auto"])

    dnl find and test if it hasn't been disabled
    if test "$check_sqlite3" != "no"; then

      dnl note if auto, and then set to yes for tests
      if test "$check_sqlite3" = "auto"; then
        sqlite3_auto=yes
        check_sqlite3=yes
      fi

      dnl look in common places
      if test "$check_sqlite3" = "yes"; then
        sqlite3_incdirs="/usr/include /usr/local/include /sw/include \
                        /usr/local/sqlite/include /usr/local/sqlite3/include \
                        /opt/sqlite/include /opt/sqlite3/include"
        sqlite3_libdirs="/usr/lib /usr/local/lib /sw/lib \
                        /usr/local/sqlite/lib /usr/local/sqlite3/lib \
                        /opt/sqlite/lib /opt/sqlite3/lib"

      dnl if a directory was given to --with-sqlite3, only look there
      else
        sqlite3_incdirs="$check_sqlite3/include $check_sqlite3/include/sqlite3"
        sqlite3_libdirs="$check_sqlite3/lib $check_sqlite3/lib/sqlite3"
        check_sqlite3=yes
      fi

      dnl override if either dir was specified and only look there
      if test ! "$check_sqlite3_incdir" = "auto"; then
        sqlite3_incdirs="$check_sqlite3_incdir $check_sqlite3_incdir/sqlite3"
      fi
      if test ! "$check_sqlite3_libdir" = "auto"; then
        sqlite3_libdirs="$check_sqlite3_libdir $check_sqlite3_libdir/sqlite3"
      fi

      dnl check for header existance
      FIND_FILE(sqlite3.h, $sqlite3_incdirs, check_sqlite3_incdir)
      if test -z "$check_sqlite3_incdir"; then
        check_sqlite3="no"
        if test ! "$sqlite3_auto" = "yes"; then
          AC_MSG_ERROR([SQLite3 headers not found])
        fi
      fi

      dnl check for lib existance
      FIND_FILE([libsqlite3.so libsqlite3.a], $sqlite3_libdirs,
                check_sqlite3_libdir)
      if test -z "$check_sqlite3_libdir"; then
        check_sqlite3="no"
        if test ! "$sqlite3_auto" = "yes"; then
          AC_MSG_ERROR([SQLite3 libraries not found])
        fi
      fi

      dnl try using it to make sure it works
      if test "$check_sqlite3" = "yes"; then
        ORIG_LDFLAGS="$LDFLAGS"
        LDFLAGS="-I$check_sqlite3_incdir -L$check_sqlite3_libdir \
                 -Wl,-R$check_sqlite3_libdir -lsqlite3"
        AC_RUN_IFELSE( [
          AC_LANG_PROGRAM( [[ 
            #include <sqlite3.h>
          ]], [[
            struct sqlite3 *db;
            sqlite3_open("", &db);
            exit(0);
          ]]
        )],
        [
          check_sqlite3=yes
          LDFLAGS="$ORIG_LDFLAGS"
        ],
        [AC_MSG_FAILURE([compiling and/or running test SQLite3 program])])
      fi
    fi
  ])

  if test "$check_sqlite3" = "yes"; then
    SQLITE3_LIBS="-lsqlite3"
    SQLITE3_INCLUDE="-I$check_sqlite3_incdir"
    SQLITE3_LDFLAGS="-L$check_sqlite3_libdir -R$check_sqlite3_libdir"
    YADA_ADD_MODULE("sqlite3")
  
    AC_SUBST(SQLITE3_LIBS)
    AC_SUBST(SQLITE3_INCLUDE)
    AC_SUBST(SQLITE3_LDFLAGS)
  else
    SQLITE3_LIBS=""
    SQLITE3_LDFLAGS=""
    SQLITE3_INCLUDE=""
  fi
])

################################################################################
################################################################################

