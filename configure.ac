
# Process this file with autoconf to produce a configure script.
AC_INIT(fincore, VERSION, mabrown@renesys.com)
AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright (c) 2008 Renesys Corporation. All Rights Reserved.])
AC_CONFIG_SRCDIR(fincore.c)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([dist-bzip2])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

# Checks for libraries.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h getopt.h fcntl.h sys/fcntl.h sys/stat.h sys/mman.h)
AC_FUNC_MMAP
AC_FUNC_GETMNTENT

dnl AC_SUBST
AC_CONFIG_FILES(Makefile)
AC_OUTPUT
