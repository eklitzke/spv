#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([spv], [0.1], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove])

CXXFLAGS="-I ../third_party/spdlog/include -I ../third_party"
AS_COMPILER_FLAG([-std=c++17],
                 [CXXFLAGS+=" -std=c++17"])
AS_COMPILER_FLAG([-fdiagnostics-color=auto],
                 [CXXFLAGS+=" -fdiagnostics-color=auto"])

PKG_CHECK_MODULES([libuv], [libuv >= 1])

AC_DEFINE([USERAGENT], ["github.com/eklitzke/spv"], [User agent to advertise.])

gitcommit=`cd $srcdir && git describe --always`
AS_IF([test x"$gitcommit" = x],
      [versionstr="spv $PACKAGE_VERSION $host_os $host_cpu"],
      [versionstr="spv $PACKAGE_VERSION (commit $gitcommit) $host_os $host_cpu"])

AC_DEFINE_UNQUOTED(
  [SPV_VERSION_STR],
  ["$versionstr"],
  [A string containing build information.])

AC_SUBST([ERROR_CFLAGS])
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
