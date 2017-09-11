if WANT_CHECKER
lib_LTLIBRARIES = libdivvun.la
divvunincludedir = $(includedir)/divvun
divvuninclude_HEADERS = checker.hpp
libdivvun_la_SOURCES = checker.cpp pipeline.cpp suggest.cpp
libdivvun_la_LIBADD =
libdivvun_CXXFLAGS = $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS)
libdivvun_la_LDFLAGS = -no-undefined
# TODO something like -version-info 49:0:0
endif


bin_PROGRAMS = divvun-suggest
bin_SCRIPTS = divvun-validate-suggest
if WANT_CHECKER
bin_PROGRAMS += divvun-checker
bin_SCRIPTS += divvun-validate-pipespec
endif

datadir = $(prefix)/share/divvun-gramcheck

# Used by divvun-validate-suggest.in:
divvun_suggestdir = $(datadir)
divvun_suggest_DATA = errors.dtd errors.rnc
EXTRA_DIST = errors.dtd errors.rnc

# divvun-suggest binary:
divvun_suggest_SOURCES = main_suggest.cpp util.hpp json.hpp suggest.cpp suggest.hpp cxxopts.hpp
divvun_suggest_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
divvun_suggest_LDADD = libdivvun.la $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS)


if WANT_CHECKER
# divvun-checker binary:
divvun_checker_SOURCES = main_checker.cpp pipeline.cpp cxxopts.hpp
divvun_checker_CXXFLAGS = $(CG3_CFLAGS) $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS) $(LIBARCHIVE_CFLAGS)
divvun_checker_LDADD = libdivvun.la $(CG3_LIBS) $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS) $(LIBARCHIVE_LIBS)
divvun_checkerdir = $(datadir)
divvun_checker_DATA = pipespec.dtd pipespec.rnc
EXTRA_DIST += pipespec.dtd pipespec.rnc
endif


# Only needed for development; we keep rnc's in version control:
if HAVE_TRANG
%.rnc: %.dtd
	test -f $(TRANG) && java -jar $(TRANG) -I dtd -O rnc $< /dev/stdout >$@.tmp
	@awk '{print}/^attlist/{print ""}' <$@.tmp >$@
	@rm -f $@.tmp
else
%.rnc: %.dtd
	@echo Could not find TRANG=$(TRANG), not updating $@
endif
