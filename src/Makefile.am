bin_PROGRAMS = divvun-feedback

#AM_CXXFLAGS = $(HFSTOSPELL_CFLAGS)

divvun_feedback_SOURCES = main.cpp feedback.cpp feedback.hpp

#what's this vs plain AM_CXXFLAGS?
divvun_feedback_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS)
divvun_feedback_LDADD = $(HFSTOSPELL_LIBS) $(HFST_LIBS)
