# Prelude
AC_INIT([lmv], [1.1], [wildwilhelm@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
# Checks for programs
AC_PROG_CXX
# Checks for libraries
AX_CHECK_GL
AX_CHECK_GLU
AX_CHECK_GLUT
# Check for library functions
AC_FUNC_STRTOD
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS(bzero memset, break)
# Output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_LANG(C++)
AC_OUTPUT

