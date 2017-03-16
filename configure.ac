AC_INIT([Divvun suggest], [0.1.0], [unhammer+apertium@mm.st], [divvun-suggest], [http://giellatekno.uit.no/bugzilla/show_bug.cgi?id=2160])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE

AC_SUBST([GLIB_CFLAGS])
CXXFLAGS="$CXXFLAGS -std=c++11 -Wall"

PKG_CHECK_MODULES([HFST], [hfst >= 3.9.0])
CXXFLAGS="$CXXFLAGS $HFST_CFLAGS"


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



AC_ARG_ENABLE([xml],
              AS_HELP_STRING([--disable-xml], [
                Disable errors.xml parsing (no longer requiring pugixml library)
              ]))
AS_IF([test "x$enable_xml" != "xno"], [
  AC_CHECK_HEADERS([pugixml.hpp])
  AS_IF([test x"$ac_cv_header_pugixml_hpp" = "xyes"], [
    AC_CHECK_LIB([pugixml], [main])
  ],[
    AC_MSG_FAILURE([Couldn't find pugixml library (use --disable-xml if you don't want to parse error.xml messages)])
  ])
  PUGIXML_LIBS="-lpugixml"
  AC_SUBST(PUGIXML_LIBS)
])

AC_PATH_PROG([XMLLINT], [xmllint], [false], [$PATH$PATH_SEPARATOR$with_xmllint/bin])
AS_IF([test x$XMLLINT = xfalse], [AC_MSG_ERROR([You don't have xmllint installed])])

AC_PATH_PROG([BASH], [bash], [false], [$PATH$PATH_SEPARATOR$with_bash/bin])
AS_IF([test x$BASH = xfalse], [AC_MSG_ERROR([You don't have bash installed])])



AC_OUTPUT(Makefile src/Makefile)
