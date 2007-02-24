dnl Process this file with autoconf to produce a configure script.
dnl AC_INIT(Defs.in)
AC_INIT([ElHam], [1.0.0], [loghyr@users.sourceforge.net], [elham])
AC_PREREQ(2.57)

AC_CONFIG_HEADER([config.h])

test "x$CFLAGS" = "x" && CFLAGS="-O2"
test "x$LDFLAGS" = "x" && LDFLAGS="-s"
AC_PROG_CC

AC_MSG_CHECKING([dirent])
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_STAT
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_HEADER_TIME
AC_PROG_INSTALL
AC_TYPE_OFF_T

AC_C_BIGENDIAN

# try some values for _FILE_OFFSET_BITS, and see what off_t sizes we
# can request.

AC_MSG_CHECKING([support for 32-bit file offsets])
AC_TRY_RUN([
#define _FILE_OFFSET_BITS 32
#include <sys/types.h>
#include <stdio.h>
main()
{
  FILE *f=fopen("conftestval","w");
  if (!f) exit(1);
  fprintf(f,"%d\n", sizeof(off_t)*8);
  exit(0);
}
], OFFSET_WHEN_32_REQUESTED=`cat conftestval`, AC_MSG_ERROR([could not determine sizeof(off_t)]))
if test $OFFSET_WHEN_32_REQUESTED = 32 ; then 
	HAVE_32_BIT_OFFSET=yes
else
	HAVE_32_BIT_OFFSET=no
fi
AC_MSG_RESULT($HAVE_32_BIT_OFFSET ($OFFSET_WHEN_32_REQUESTED))

AC_MSG_CHECKING([support for 64-bit file offsets])
AC_TRY_RUN([
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <stdio.h>
main()
{
  FILE *f=fopen("conftestval","w");
  if (!f) exit(1);
  fprintf(f,"%d\n", sizeof(off_t)*8);
  exit(0);
}
], OFFSET_WHEN_64_REQUESTED=`cat conftestval`, AC_MSG_ERROR([could not determine sizeof(off_t)]))
if test $OFFSET_WHEN_64_REQUESTED = 64 ; then 
	HAVE_64_BIT_OFFSET=yes
else
	HAVE_64_BIT_OFFSET=no
fi
AC_MSG_RESULT($HAVE_64_BIT_OFFSET ($OFFSET_WHEN_64_REQUESTED))

if test $HAVE_64_BIT_OFFSET = yes ; then
	LARGEST_OFFSET=64
elif test $HAVE_32_BIT_OFFSET = yes ; then
	LARGEST_OFFSET=32
else
	AC_MSG_ERROR([I cannot figure out how to request off_t of either 32 or 64 bits size])
fi

AC_SUBST(HAVE_64_BIT_OFFSET)
AC_SUBST(HAVE_32_BIT_OFFSET)
AC_SUBST(LARGEST_OFFSET)

AC_CHECK_FUNCS([gettimeofday memset mkdir rmdir strchr strerror strrchr])
AC_CHECK_HEADERS([sys/mount.h sys/statvfs.h sys/vfs.h])

AC_DEFUN(AC_CHECK_STRUCT_STATVFS_T,
[
AC_MSG_CHECKING([for struct statvfs_t])
AC_CACHE_VAL(ac_cv_func_struct_statvfs_t,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
ac_libs_safe="$LIBS"
LIBS="$LIBS $LIBUCB"
AC_TRY_LINK([
# include <sys/statvfs.h>
int statvfs ();
],
[
statvfs_t fsd;
],
ac_cv_func_struct_statvfs_t=yes,
ac_cv_func_struct_statvfs_t=no)
LIBS="$ac_libs_safe"
AC_LANG_RESTORE
])
AC_MSG_RESULT($ac_cv_func_struct_statvfs_t)
if eval "test \"`echo $ac_cv_func_struct_statvfs_t`\" = yes"; then
  AC_DEFINE(HAVE_STRUCT_STATVFS_T, 1, [Define if you have the struct statvfs_t])
fi
])

AC_CHECK_STRUCT_STATVFS_T

BINDIR=`pwd`/bin
AC_SUBST(BINDIR)
MANDIR=`pwd`/man
AC_SUBST(MANDIR)
EXE=''
AC_SUBST(EXE)
UNIX=yes
AC_SUBST(UNIX)
WINDOWS=no
AC_SUBST(WINDOWS)
AC_OUTPUT(Defs)
