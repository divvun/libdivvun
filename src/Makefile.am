datadir = $(prefix)/share/divvun-gramcheck

# divvun-suggest binary:
divvun_suggest_SOURCES  = main_suggest.cpp suggest.cpp
divvun_suggest_LDADD    = $(HFST_LIBS)   $(PUGIXML_LIBS)
divvun_suggest_CXXFLAGS = $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
bin_PROGRAMS            = divvun-suggest

# divvun-gen-sh binary
divvun_gen_sh_SOURCES  = main_gen_sh.cpp pipespec.cpp
divvun_gen_sh_LDADD    = $(PUGIXML_LIBS)   $(LIBARCHIVE_LIBS)
divvun_gen_sh_CXXFLAGS = $(PUGIXML_CFLAGS) $(LIBARCHIVE_CFLAGS)
bin_PROGRAMS          += divvun-gen-sh

bin_SCRIPTS         = divvun-gen-xmlschemas divvun-validate-suggest divvun-validate-pipespec
# Used by the .in files of bin_SCRIPTS:
divvun_suggestdir   = $(datadir)
divvun_suggest_DATA = errors.dtd errors.rnc pipespec.dtd pipespec.rnc
EXTRA_DIST          = errors.dtd errors.rnc pipespec.dtd pipespec.rnc
# We make pipespec-stuff even if --disable-checker; it doesn't add to dependencies

# divvun-blanktag binary:
divvun_blanktag_SOURCES  = main_blanktag.cpp blanktag.cpp
divvun_blanktag_LDADD    = $(HFST_LIBS)
divvun_blanktag_CXXFLAGS = $(HFST_CFLAGS)
bin_PROGRAMS            += divvun-blanktag

if WANT_CGSPELL
# divvun-cgspell binary:
divvun_cgspell_SOURCES  = main_cgspell.cpp cgspell.cpp
divvun_cgspell_LDADD    = $(HFSTOSPELL_LIBS)
divvun_cgspell_CXXFLAGS = $(HFSTOSPELL_CFLAGS)
bin_PROGRAMS           += divvun-cgspell
endif

if WANT_CHECKER
lib_LTLIBRARIES       = libdivvun.la
divvunincludedir      = $(includedir)/divvun
divvuninclude_HEADERS = checker.hpp checkertypes.hpp
libdivvun_la_SOURCES  = checker.cpp pipeline.cpp pipespec.cpp suggest.cpp cgspell.cpp blanktag.cpp
# would've liked to put -lstdc++fs in LIBADD for <experimental/filesystem>, but macos
libdivvun_la_LIBADD   =              $(CG3_LIBS)   $(divvun_gen_sh_LDADD)    $(divvun_suggest_LDADD)    $(divvun_cgspell_LDADD)
libdivvun_CXXFLAGS    =              $(CG3_CFLAGS) $(divvun_gen_sh_CXXFLAGS) $(divvun_suggest_CXXFLAGS) $(divvun_cgspell_CXXFLAGS)
libdivvun_la_LDFLAGS  = -no-undefined
# TODO something like -version-info 49:0:0

bin_PROGRAMS         += divvun-checker

# divvun-checker binary:
divvun_checker_SOURCES  = main_checker.cpp
divvun_checker_LDADD    = libdivvun.la $(libdivvun_la_LIBADD)
divvun_checker_CXXFLAGS =              $(libdivvun_CXXFLAGS)
divvun_checkerdir       = $(datadir)
endif

# Only needed for development; we keep rnc's in version control:
if HAVE_TRANG
%.rnc: %.dtd
	test -f $(TRANG) && java -jar $(TRANG) -I dtd -O rnc $< /dev/stdout >$@.tmp
	@awk 'BEGIN{print "# THIS IS A GENERATED FILE, DO NOT EDIT!"}{print}/^attlist/{print ""}' <$@.tmp >$@
	@rm -f $@.tmp
else
%.rnc: %.dtd
	@echo Could not find TRANG=$(TRANG), not updating $@
endif
