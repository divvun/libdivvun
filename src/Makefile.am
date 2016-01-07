bin_PROGRAMS = divvun-suggest

#AM_CXXFLAGS = $(HFSTOSPELL_CFLAGS)

divvun_suggest_SOURCES = main.cpp suggest.cpp suggest.hpp

#what's this vs plain AM_CXXFLAGS?
divvun_suggest_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS)
divvun_suggest_LDADD = $(HFSTOSPELL_LIBS) $(HFST_LIBS)
