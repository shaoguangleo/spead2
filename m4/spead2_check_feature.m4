## Check whether a library is available. Unlike AC_CHECK_LIB, this supports
## C++. It takes a header file, a library to link against, and a code snippet
## to build inside main().
## Unlike other autoconf tests, action-if-found defaults to nothing

#serial 1

AC_DEFUN([_SPEAD2_INCLUDE], [#include <$1>
])

# SPEAD2_CHECK_FEATURE(NAME, DESCRIPTION, HEADERS, LIBRARY, BODY,
#                      [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [EXTRA-PREAMBLE])
# -------------------------------------------------------------------------------
AC_DEFUN([SPEAD2_CHECK_FEATURE], [
    AS_VAR_PUSHDEF([cv], [spead2_cv_$1])
    AC_CACHE_CHECK([for $2], [cv], [
        spead2_save_LIBS=$LIBS
        m4_ifval([$4], [LIBS="-l$4 $LIBS"],)
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([$8
                              m4_map_args_w([$3], [_SPEAD2_INCLUDE(], [)])], [$5])],
            [AS_VAR_SET([cv], [yes])],
            [AS_VAR_SET([cv], [no])])
        LIBS=$spead2_save_LIBS
    ])
    AS_VAR_IF([cv], [yes], [$6], [$7])
    AS_VAR_POPDEF([cv])
])

# SPEAD2_CHECK_LIB(HEADERS, LIBRARY, BODY, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
# -----------------------------------------------------------------------------
AC_DEFUN([SPEAD2_CHECK_LIB], [SPEAD2_CHECK_FEATURE([lib_$2], [-l$2], [$1], [$2], [$3], [$4], [$5])])

# SPEAD2_CHECK_HEADER(HEADER, LIBRARY, BODY, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
# -----------------------------------------------------------------------------
AC_DEFUN([SPEAD2_CHECK_HEADER], [SPEAD2_CHECK_FEATURE([header_$1], [$1], [$1], [$2], [$3],[$4], [$5])])