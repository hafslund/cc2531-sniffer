#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cc2531-sniffer], [0.1])
AC_CONFIG_SRCDIR([cc2531-sniffer.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AM_PROG_CC_C_O

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

PKG_CHECK_MODULES(USB, libusb-1.0 >= 1.0.10)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

