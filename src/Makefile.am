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


# libdivvun-suggest:
lib_LTLIBRARIES = libdivvun-suggest.la
libdivvun_suggest_la_SOURCES = suggest.cpp
include_HEADERS = suggest.hpp
# divvun-suggest binary:
divvun_suggest_SOURCES = main_suggest.cpp util.hpp json.hpp suggest.cpp suggest.hpp cxxopts.hpp
divvun_suggest_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
divvun_suggest_LDADD = $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS)


if WANT_CHECKER
# libdivvun-checker, TODO:
# lib_LTLIBRARIES += libdivvun-checker.la
# libdivvun_checker_la_SOURCES = checker.cpp
# include_HEADERS += checker.hpp
# divvun-checker binary:
divvun_checker_SOURCES = main_checker.cpp checker.cpp cxxopts.hpp
divvun_checker_CXXFLAGS = -I../third-party/cg3/include -I../third-party/cg3/src $(CG3_CFLAGS) $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS) $(LIBARCHIVE_CFLAGS)
divvun_checker_LDADD = libdivvun-suggest.la -licui18n -licudata -licuio -licuuc $(CG3_LIBS) $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS) $(LIBARCHIVE_LIBS)
# TODO: shouldn't need to link to icu once we get a real cg3 api and can do that stuff there
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
