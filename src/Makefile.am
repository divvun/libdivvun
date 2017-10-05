datadir = $(prefix)/share/divvun-gramcheck

# divvun-suggest binary:
divvun_suggest_SOURCES  = main_suggest.cpp util.hpp json.hpp suggest.cpp suggest.hpp cxxopts.hpp
divvun_suggest_LDADD    = $(HFST_LIBS)   $(PUGIXML_LIBS)
divvun_suggest_CXXFLAGS = $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
bin_PROGRAMS            = divvun-suggest

bin_SCRIPTS         = divvun-gen-xmlschemas divvun-validate-suggest divvun-validate-pipespec
# Used by the .in files of bin_SCRIPTS:
divvun_suggestdir   = $(datadir)
divvun_suggest_DATA = errors.dtd errors.rnc pipespec.dtd pipespec.rnc
EXTRA_DIST          = errors.dtd errors.rnc pipespec.dtd pipespec.rnc
# We make pipespec-stuff even if --disable-checker; it doesn't add to dependencies

if WANT_CGSPELL
# divvun-cgspell binary:
divvun_cgspell_SOURCES  = main_cgspell.cpp cgspell.cpp cgspell.hpp cxxopts.hpp
divvun_cgspell_LDADD    = $(HFSTOSPELL_LIBS)   $(PUGIXML_LIBS)
divvun_cgspell_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(PUGIXML_CFLAGS)
bin_PROGRAMS           += divvun-cgspell
endif

if WANT_CHECKER
lib_LTLIBRARIES       = libdivvun.la
divvunincludedir      = $(includedir)/divvun
divvuninclude_HEADERS = checker.hpp checkertypes.hpp
libdivvun_la_SOURCES  = checker.cpp pipeline.cpp suggest.cpp
# would've liked to put -lstdc++fs in LIBADD for <experimental/filesystem>, but macos
libdivvun_la_LIBADD   =              $(CG3_LIBS)   $(LIBARCHIVE_LIBS)   $(divvun_suggest_LDADD)
libdivvun_CXXFLAGS    =              $(CG3_CFLAGS) $(LIBARCHIVE_CFLAGS) $(divvun_suggest_CXXFLAGS)
libdivvun_la_LDFLAGS  = -no-undefined
# TODO something like -version-info 49:0:0

bin_PROGRAMS         += divvun-checker divvun-gen-sh

# divvun-checker binary:
divvun_checker_SOURCES  = main_checker.cpp pipeline.cpp cxxopts.hpp
divvun_checker_LDADD    = libdivvun.la $(libdivvun_la_LIBADD)
divvun_checker_CXXFLAGS =              $(libdivvun_CXXFLAGS)
divvun_checkerdir       = $(datadir)

# divvun-gen-sh binary:
divvun_gen_sh_SOURCES  = main_gen_sh.cpp pipeline.cpp cxxopts.hpp
divvun_gen_sh_LDADD    = libdivvun.la $(libdivvun_la_LIBADD)
divvun_gen_sh_CXXFLAGS =              $(libdivvun_CXXFLAGS)
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
