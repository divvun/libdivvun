AC_INIT([Divvun gramcheck], [0.1.0], [unhammer+apertium@mm.st], [divvun-gramcheck], [https://github.com/divvun/divvun-gramcheck])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE

# AC_PROG_LIBTOOL deprecated in favour of LT_INIT
LT_INIT

AC_SUBST([GLIB_CFLAGS])
CXXFLAGS="$CXXFLAGS -std=c++11 -Wall"

PKG_CHECK_MODULES([HFST], [hfst >= 3.12.2])
CXXFLAGS="$CXXFLAGS $HFST_CFLAGS"

PKG_CHECK_MODULES([CG3], [cg3 >= 1.1.2.12361])
CXXFLAGS="$CXXFLAGS $CG3_CFLAGS"


AC_PROG_CXX

dnl check for C++11 support:
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_SOURCE(
  [[template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);]])],,
  AC_MSG_FAILURE(['$CXX $CXXFLAGS' does not accept ISO C++11]))


# XML parsing:
AC_ARG_ENABLE([xml],
              AS_HELP_STRING([--disable-xml],
                             [Disable errors.xml parsing (no longer requiring pugixml library) @<:@default=check@:>@]),
              [enable_xml=$enableval],
              [enable_xml=check])
AS_IF([test "x$enable_xml" != xno],
      [AC_CHECK_HEADERS([pugixml.hpp])
       AS_IF([test x"$ac_cv_header_pugixml_hpp" = "xyes"],
             [AC_CHECK_LIB([pugixml], [main])],
             [AC_MSG_FAILURE([Couldn't find pugixml library (use --disable-xml if you don't want to parse error.xml messages and don't need --enable-checker)])])
       PUGIXML_LIBS="-lpugixml"
       AC_SUBST(PUGIXML_LIBS)
       enable_xml=yes])
AM_CONDITIONAL([HAVE_LIBPUGIXML], [test x"$PUGIXML_LIBS" != x])

# Checker:
AC_ARG_ENABLE([checker],
              [AS_HELP_STRING([--enable-checker],
                              [build divvun-checker, for running full pipelines from a library, with no IPC @<:@default=check@:>@])],
              [enable_checker=$enableval],
              [enable_checker=check])
AS_IF([test "x$enable_checker" != xno],
      [AS_IF([test x"$enable_xml" = xno],
             [AC_MSG_FAILURE([--enable-xml required for checker])])
       PKG_CHECK_MODULES([LIBARCHIVE], [libarchive > 3],
                  [AC_DEFINE([HAVE_LIBARCHIVE], [1], [Use archives])
                   enable_checker=yes],
                  [PKG_CHECK_MODULES([LIBARCHIVE], [libarchive > 2],
                              [AC_DEFINE([HAVE_LIBARCHIVE], [1], [Use archives])
	                       AC_DEFINE([USE_LIBARCHIVE_2], [1], [Use libarchive2])
                               enable_checker=yes],
                              [AS_IF([test x$enable_checker = xyes],
                                     [AC_MSG_ERROR([checker support requires either libarchive or libarchive2 for opening zipped pipespec archives])
                                      enable_checker=no],
                                     [enable_checker=no])])])])
AM_CONDITIONAL([WANT_CHECKER], [test x$enable_checker != xno])

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
AM_CONDITIONAL([WANT_CGSPELL], [test x$enable_cgspell != xno])


AC_CONFIG_FILES([src/divvun-gen-xmlschemas],    [chmod +x src/divvun-gen-xmlschemas])
AC_CONFIG_FILES([src/divvun-validate-suggest],  [chmod +x src/divvun-validate-suggest])
AC_CONFIG_FILES([src/divvun-validate-pipespec], [chmod +x src/divvun-validate-pipespec])

AC_PATH_PROG([XMLLINT], [xmllint], [false], [$PATH$PATH_SEPARATOR$with_xmllint/bin])
AS_IF([test x$XMLLINT = xfalse], [AC_MSG_ERROR([You don't have xmllint installed])])

AC_PATH_PROG([BASH], [bash], [false], [$PATH$PATH_SEPARATOR$with_bash/bin])
AS_IF([test x$BASH = xfalse], [AC_MSG_ERROR([You don't have bash installed])])

AC_ARG_VAR([TRANG], [Path to trang.jar (dev dependency for updating DTD)])
AC_ARG_WITH([trang], [AS_HELP_STRING([--with-trang],[Path to trang.jar (dev dependency for updating DTD)])], [TRANG=$withval], [TRANG=])
AM_CONDITIONAL([HAVE_TRANG], [test x"$TRANG" != x])

AC_OUTPUT(Makefile
          src/Makefile
          test/suggest/Makefile
          test/checker/Makefile
          test/cgspell/Makefile)

cat <<EOF
-- Building $PACKAGE_STRING
    * checker support: $enable_checker
    * cgspell support: $enable_cgspell
    * xml support: $enable_xml
    * trang: $TRANG
EOF
