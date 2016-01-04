AC_INIT([Divvun feedback], [0.1.0], [unhammer+apertium@mm.st])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE

dnl HFST_REQUIRED_VERSION=3.9.0
dnl PKG_CHECK_MODULES(HFST, hfst >= $HFST_REQUIRED_VERSION)
dnl AC_SUBST(HFST_CFLAGS)
dnl AC_SUBST(HFST_LIBS)

PKG_CHECK_MODULES([HFSTOSPELL], [hfstospell >= 0.2])
# what's this vs divvun_feedback_CXXFLAGS?
CXXFLAGS="$CXXFLAGS $HFSTOSPELL_CFLAGS"

AC_PROG_CXX

AC_OUTPUT(Makefile src/Makefile)
