#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


AC_PREREQ([2.52])
AC_INIT(dtmfcmd, 0.1.0, http://www.binaryvision.co.il)
AC_CONFIG_SRCDIR([src/control.h])
AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES(GLIB, glib-2.0)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h getopt.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([regcomp])

#LIBRARY flags for the make file.
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
scripts/Makefile
])

AC_OUTPUT
