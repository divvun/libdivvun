AC_INIT([Divvun gramcheck], [0.3.9], [unhammer+apertium@mm.st], [divvun-gramcheck], [https://github.com/divvun/divvun-gramcheck])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE

dnl TODO: tests require zip and gawk (not mawk)

# AC_PROG_LIBTOOL deprecated in favour of LT_INIT
LT_INIT

AC_SUBST([GLIB_CFLAGS])

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG(C++)

CXXFLAGS="$CXXFLAGS -Wall -Werror=switch"

PKG_CHECK_MODULES([HFST], [hfst >= 3.12.2])
CXXFLAGS="$CXXFLAGS $HFST_CFLAGS"

AS_CASE([$HFST_CFLAGS],
        [*@GLIB_CFLAGS@*],
        [x=$(pkg-config --debug 2>&1|grep -o "in file .*hfst\.pc.*")
         AC_MSG_ERROR([buggy hfst.pc, please remove the string @GLIB_CFLAGS@ from the Cflags line $x])])

PKG_CHECK_MODULES([CG3], [cg3 >= 1.1.2.12361])
CXXFLAGS="$CXXFLAGS $CG3_CFLAGS"

AX_CHECK_COMPILE_FLAG([-Wall], [], AC_MSG_ERROR([compiler doesn't accept -Wall - check config.log]))

AX_CHECK_COMPILE_FLAG([-fstack-protector-strong], [CXXFLAGS="$CXXFLAGS -fstack-protector-strong"])


dnl ====================
dnl Checks C++11 support
dnl ====================
AX_CHECK_COMPILE_FLAG([-std=c++17], [CXXFLAGS="$CXXFLAGS -std=c++17"], [
 AX_CHECK_COMPILE_FLAG([-std=c++1z], [CXXFLAGS="$CXXFLAGS -std=c++1z"], [
  AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXFLAGS="$CXXFLAGS -std=c++14"], [
   AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"], [
    AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS="$CXXFLAGS -std=c++11"], [
     AC_MSG_ERROR([compiler doesn't accept any -std=c++11, please upgrade your compiler])])
   ])
  ])
 ])
])


dnl ==================
dnl Checks for pugixml
dnl ==================
AC_ARG_ENABLE([xml],
              AS_HELP_STRING([--disable-xml],
                             [Disable errors.xml parsing (no longer requiring pugixml library) @<:@default=check@:>@]),
              [enable_xml=$enableval],
              [enable_xml=check])
AS_IF([test "x$enable_xml" != xno],
      [AC_CHECK_HEADERS([pugixml.hpp])
       AS_IF([test x"$ac_cv_header_pugixml_hpp" = "xyes"],
             [AC_CHECK_LIB([pugixml], [main])], dnl TODO: fail if this test fails (no libpugixml.so)
             [AC_MSG_ERROR([cannot find pugixml library (use --disable-xml if you don't want to parse error.xml messages and don't need --enable-checker)])])
       PUGIXML_LIBS="-lpugixml"
       AC_SUBST(PUGIXML_LIBS)
       enable_xml=yes])
AM_CONDITIONAL([HAVE_LIBPUGIXML], [test x"$PUGIXML_LIBS" != x])


dnl ==================
dnl Checks for cgspell
dnl ==================
AC_ARG_ENABLE([cgspell],
              [AS_HELP_STRING([--enable-cgspell],
                              [build divvun-cgspell, for spelling unknown (or even known) words from CG streams @<:@default=check@:>@])],
              [enable_cgspell=$enableval],
              [enable_cgspell=check])
AS_IF([test "x$enable_cgspell" != xno],
      [PKG_CHECK_MODULES([HFSTOSPELL], [hfstospell >= 0.4.3],
                         [enable_cgspell=yes],
                         [AS_IF([test x$enable_cgspell = xyes],
                                [AC_MSG_ERROR([cgspell support requires libhfstospell / hfst-ospell-dev])
                                 enable_cgspell=no],
                                [enable_cgspell=no])])])
AM_CONDITIONAL([HAVE_CGSPELL], [test x$enable_cgspell = xyes])
AS_IF([test "x$enable_cgspell" = xyes], [AC_DEFINE([HAVE_CGSPELL])])


dnl =====================
dnl Checks for libarchive
dnl =====================
PKG_CHECK_MODULES([LIBARCHIVE], [libarchive > 3],
                  [AC_DEFINE([HAVE_LIBARCHIVE], [1], [Use archives])
                   have_libarchive=yes],
                  [PKG_CHECK_MODULES([LIBARCHIVE], [libarchive > 2],
                              [AC_DEFINE([HAVE_LIBARCHIVE], [1], [Use archives])
                               have_libarchive=yes
	                       AC_DEFINE([USE_LIBARCHIVE_2], [1], [libarchive is version 2])],
                               [have_libarchive=no])])

dnl ============================
dnl Checks for checker/libdivvun
dnl ============================
AC_ARG_ENABLE([checker],
              [AS_HELP_STRING([--enable-checker],
                              [build divvun-checker, for running full pipelines from a library, with no IPC @<:@default=check@:>@])],
              [enable_checker=$enableval],
              [enable_checker=check])
AS_CASE([$enable_checker],
        [no], [],
        [yes], [ dnl Explicitly enabled -- fail if missing deps
                 AS_CASE([$enable_xml],
                         [no], [AC_MSG_ERROR([you need --enable-xml if you want --enable-checker

                    Please run again with --enable-xml and ensure its dependencies are installed.
                    ])])
                 AS_CASE([$have_libarchive],
                         [no], [AC_MSG_ERROR([you need libarchive if you want --enable-checker])])
        ],
        [check], [ dnl Enable if deps are there
                 AS_CASE([$enable_xml],
                         [no],  [AC_MSG_WARN([No --enable-xml, disabling checker])
                                 enable_checker=no])
                 AS_CASE([$have_libarchive],
                         [no],  [AC_MSG_WARN([No libarchive, disabling checker])
                                 enable_checker=no])
                 AS_CASE([$enable_checker],
                         [check], [enable_checker=yes])
        ])
AM_CONDITIONAL([HAVE_CHECKER], [test "x$enable_checker" != xno])
AC_DEFINE([HAVE_CHECKER])


dnl ===============
dnl Checks for SWIG
dnl ===============
AC_PATH_PROG([SWIG], [swig])
AS_IF([test ! "$SWIG"],
      [AC_PATH_PROG([SWIG], [swig3.0])])
AM_CONDITIONAL([HAVE_SWIG], [test "$SWIG"])
AS_IF([test ! "$SWIG"],
      [AC_MSG_WARN([SWIG not found. SWIG is required to build divvun-gramcheck python bindings.])])


dnl =================
dnl Checks for Python
dnl =================

dnl The test must instead add the result of `python3-config --includes` to the build flags and only `#include <Python.h>`

AM_PATH_PYTHON([3.0],
               [],
               [AC_MSG_WARN([Python not found. Python is required to build divvun-gramcheck python bindings.])])
AS_IF([test "$PYTHON" != :],
      [AC_SUBST(PYTHONINCLUDE,["`python3-config --includes`"])
       CPPFLAGS="$CPPFLAGS $PYTHONINCLUDE"
       AC_CHECK_HEADERS([Python.h],
                        [have_python_header=true],
                        [AC_MSG_WARN([Python.h header file not found. Python development files are required to build the divvun-gramcheck python bindings.])],
                        [])])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != : -a "x$have_python_header" = xtrue])

AC_ARG_ENABLE([python-bindings],
              AS_HELP_STRING([--disable-python-bindings],
                             [build python bindings (default=enabled)]),
              [enable_python_bindings=$enableval],
              [enable_python_bindings=check])
AS_CASE([$enable_python_bindings],
        [no], [],
        [yes], [ dnl Explicitly enabled -- fail if missing deps
                 AS_IF([test "x$enable_checker" != xyes],
                       [AC_MSG_ERROR([cannot build python bindings without building checker library (--enable-checker)])])
                 AS_IF([test "$PYTHON" = : -o "x$have_python_header" != xtrue],
                       [AC_MSG_ERROR([cannot find python/python headers (use --disable-python-bindings if you don't need the bindings)])])
                 AS_IF([test ! "$SWIG"],
                       [AC_MSG_ERROR([cannot find SWIG (use --disable-python-bindings if you don't need the bindings)])])
        ],
        [check], [ dnl Enable if deps are there
             dnl Not explicitly enabled, enable if we have deps:
             AS_IF([test "$SWIG" -a "$PYTHON" != : -a "x$have_python_header" = xtrue -a "x$enable_checker" = xyes],
                   [enable_python_bindings=yes],
                   [enable_python_bindings=no])
        ])
AM_CONDITIONAL([HAVE_PYTHON_BINDINGS], [test "x$enable_python_bindings" != xno])


dnl Install parameters for Python, overridable by Debian packaging:
AC_ARG_VAR([PYTHON_INSTALL_PARAMS], [Parameters to pass to the Python 3 module install step])
if test "x$PYTHON_INSTALL_PARAMS" = "x"
then
  PYTHON_INSTALL_PARAMS="--prefix=\$(prefix) --root=\$(DESTDIR)/"
fi




dnl ===============================
dnl Checks for xmllint, bash, trang
dnl ===============================

AC_PATH_PROG([XMLLINT], [xmllint], [false], [$PATH$PATH_SEPARATOR$with_xmllint/bin])
AS_IF([test x$XMLLINT = xfalse], [AC_MSG_ERROR([you don't have xmllint installed])])

AC_PATH_PROG([BASH], [bash], [false], [$PATH$PATH_SEPARATOR$with_bash/bin])
AS_IF([test x$BASH = xfalse], [AC_MSG_ERROR([you don't have bash installed])])

AC_ARG_VAR([TRANG], [Path to trang.jar (dev dependency for updating DTD)])
AC_ARG_WITH([trang], [AS_HELP_STRING([--with-trang],[Path to trang.jar (dev dependency for updating DTD)])], [TRANG=$withval], [TRANG=])
AM_CONDITIONAL([HAVE_TRANG], [test x"$TRANG" != x])


dnl ===========================
dnl Create the files from .in's
dnl ===========================

AC_CONFIG_FILES([src/divvun-gen-xmlschemas],    [chmod +x src/divvun-gen-xmlschemas])
AC_CONFIG_FILES([src/divvun-validate-suggest],  [chmod +x src/divvun-validate-suggest])
AC_CONFIG_FILES([src/divvun-validate-pipespec], [chmod +x src/divvun-validate-pipespec])

AC_OUTPUT([Makefile
           src/Makefile
           src/version.hpp
           python/Makefile
	   python/setup.py
           test/suggest/Makefile
           test/checker/Makefile
           test/checker/run-python-bindings
           test/blanktag/Makefile
           test/cgspell/Makefile],
          [chmod +x test/checker/run-python-bindings])

cat <<EOF
-- Building $PACKAGE_STRING
    * checker support: $enable_checker
    * cgspell support: $enable_cgspell
    * xml support: $enable_xml
    * python/swig bindings: $enable_python_bindings
    * trang: $TRANG
    * python: $PYTHON
    * swig: $SWIG
EOF
