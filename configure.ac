#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([spv], [0.1], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_LANG([C++])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h stdint.h stdlib.h string.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_STRTOD
AC_CHECK_FUNCS([clock_gettime dup2 getpagesize gettimeofday localeconv localtime_r memmove memset mkdir realpath rmdir strerror strstr strtoul utime])

CXXFLAGS+=" -Wall -I ../third_party/spdlog/include -I ../third_party"
AS_COMPILER_FLAG([-std=c++17],
                 [CXXFLAGS+=" -std=c++17"])
AS_COMPILER_FLAG([-fdiagnostics-color=auto],
                 [CXXFLAGS+=" -fdiagnostics-color=auto"])

PKG_CHECK_MODULES([libuv], [libuv >= 1])

AC_DEFINE_UNQUOTED([USER_AGENT], ["eklitzke/$PACKAGE_STRING"], [Our user agent.])
AC_DEFINE([PROTOCOL_MAGIC], [0x0709110B], [P2P protocol magic.])
AC_DEFINE([PROTOCOL_PORT], [18333], [P2P protocol port.])

# See https://bitcoin.org/en/developer-reference#protocol-versions for the meaning of this
AC_DEFINE([PROTOCOL_VERSION], [70012], [P2P protocol version.])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
