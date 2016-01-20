AC_INIT([Divvun suggest], [0.1.0], [unhammer+apertium@mm.st])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE

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


AC_CHECK_HEADERS([pugixml.hpp])
if test x"$ac_cv_header_pugixml_hpp" = xyes; then
  AC_CHECK_LIB([pugixml], [main])
fi
PUGIXML_LIBS="-lpugixml"
AC_SUBST(PUGIXML_LIBS)


AC_OUTPUT(Makefile src/Makefile)
