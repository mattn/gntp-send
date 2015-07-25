AC_PREREQ([2.69])
AC_INIT([gntp-send],m4_esyscmd(cat VERSION | tr -d '\n'),[mattn.jp@gmail.com])

AM_INIT_AUTOMAKE([foreign -Wall])

AC_CONFIG_SRCDIR([src/growl.c])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT

LT_INIT

# smart output
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
