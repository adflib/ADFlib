#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

# version details - use utils/bump-version to update it
m4_define([adflib_version],[0.10.2])
m4_define([adflib_lt_version],[3:2:0])
m4_define([adflib_date],[2025-05-16])

AC_INIT([adflib],[adflib_version])

AC_CONFIG_SRCDIR([src/adf_env.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

#AM_INIT_AUTOMAKE([1.9 tar-pax])  # https://noiselabs.io/tar-file-name-is-too-long-max-99/
AM_INIT_AUTOMAKE([1.9 tar-ustar])
AM_SILENT_RULES([yes])

AC_ARG_ENABLE([examples],
              [  --enable-examples    Build examples],
              [case "${enableval}" in
                yes) examples=true ;;
                no)  examples=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
               esac],
              [examples=true])

AM_CONDITIONAL([EXAMPLES], [test x$examples = xtrue])

AC_ARG_ENABLE([regtests],
              [  --enable-regtests    Build regression tests],
              [case "${enableval}" in
                yes) regtests=true ;;
                no)  regtests=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-regtests]) ;;
               esac],
              [regtests=true])

AM_CONDITIONAL([REGTESTS], [test x$regtests = xtrue])

#
# native
#
AC_CANONICAL_HOST

# detect first
echo "Host OS: ${host_os}"
case "${host_os}" in
     linux*)
	NATIVE_DIR=linux
	;;
     win32*)
	NATIVE_DIR=win32
	;;
     cygwin*)
	NATIVE_DIR=win32
	;;
     darwin*)
	NATIVE_DIR=generic
	;;
     *)
        NATIVE_DIR=generic
	;;
esac

# ... then check if enforced
AC_ARG_ENABLE([native_generic],
              [  --enable-native-generic    Enable generic native devices],
              [case "${enableval}" in
                yes) NATIVE_DIR=generic ;;
                no)  ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-native-generic]) ;;
               esac],
              [])

AC_ARG_ENABLE([native_linux],
              [  --enable-native-linux    Enable Linux native devices],
              [case "${enableval}" in
                yes) NATIVE_DIR=linux ;;
                no)  ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-native-linux]) ;;
               esac],
              [])

AC_ARG_ENABLE([native_win32],
              [  --enable-native-win32    Enable Win32 native devices],
              [case "${enableval}" in
                yes) NATIVE_DIR=win32 ;;
                no)  ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-native-win32]) ;;
               esac],
              [])

echo "Native device: ${NATIVE_DIR}"
AM_CONDITIONAL([NATIVE_GENERIC], [test x${NATIVE_DIR} = xgeneric])
AM_CONDITIONAL([NATIVE_LINUX],   [test x${NATIVE_DIR} = xlinux])
AM_CONDITIONAL([NATIVE_WIN32],   [test x${NATIVE_DIR} = xwin32])

# Enable/disabke experimental salvage features
AC_ARG_ENABLE([salvage_dircache],
              [  --enable-salvage-dircache    Enable salvage on volumes with dircache (EXPERIMENTAL!)],
              [case "${enableval}" in
                yes) salvage_dircache=true ;;
                no)  salvage_dircache=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-salvage-dircache]) ;;
               esac],
              [salvage_dircache=true])

AM_CONDITIONAL([SALVAGE_DIRCACHE], [test x$salvage_dircache = xtrue])
echo "Salvage on dircache: ${salvage_dircache}"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES([CHECK], [check >= 0.11.0], [tests=yes], [tests=no])
AM_CONDITIONAL([TESTS], [test x${tests} = xyes])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Check functions
AC_CHECK_FUNCS(chmod, AC_DEFINE([HAVE_CHMOD], [1]))
AC_CHECK_FUNCS(getopt, AC_DEFINE([HAVE_GETOPT], [1]))
AC_CHECK_FUNCS(strnlen, AC_DEFINE([HAVE_STRNLEN], [1]))
AC_CHECK_FUNCS(strndup, AC_DEFINE([HAVE_STRNDUP], [1]))
AC_CHECK_FUNCS(mempcpy, AC_DEFINE([HAVE_MEMPCPY], [1]))
AC_CHECK_FUNCS(stpncpy, AC_DEFINE([HAVE_STPNCPY], [1]))

# Version
AC_SUBST([ADFLIB_VERSION], [adflib_version])
AC_SUBST([ADFLIB_LT_VERSION], [adflib_lt_version])
AC_SUBST([ADFLIB_DATE], [adflib_date])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 doc/Makefile
                 examples/Makefile
                 tests/Makefile
                 tests/config.sh
                 tests/examples/Makefile
                 tests/examples2/Makefile
                 tests/regr/Makefile
                 tests/unit/Makefile
                 adflib.pc])
AC_OUTPUT
