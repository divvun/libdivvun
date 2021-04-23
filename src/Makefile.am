datadir = $(prefix)/share/divvun-gramcheck
AM_CPPFLAGS = -DPREFIX="\"$(prefix)\""

# divvun-suggest binary:
divvun_suggest_SOURCES  = main_suggest.cpp suggest.cpp
divvun_suggest_LDADD    = $(HFST_LIBS)   $(PUGIXML_LIBS)
divvun_suggest_CXXFLAGS = $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
bin_PROGRAMS            = divvun-suggest
dist_man_MANS           = divvun-suggest.1

if HAVE_LIBPUGIXML
# divvun-gen-sh binary
divvun_gen_sh_SOURCES  = main_gen_sh.cpp pipespec.cpp
divvun_gen_sh_LDADD    = $(PUGIXML_LIBS)   $(LIBARCHIVE_LIBS)
divvun_gen_sh_CXXFLAGS = $(PUGIXML_CFLAGS) $(LIBARCHIVE_CFLAGS)
bin_PROGRAMS          += divvun-gen-sh
dist_man_MANS         += divvun-gen-sh.1
endif

bin_SCRIPTS         = divvun-gen-xmlschemas   divvun-validate-suggest   divvun-validate-pipespec
dist_man_MANS      += divvun-gen-xmlschemas.1 divvun-validate-suggest.1 divvun-validate-pipespec.1
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
dist_man_MANS           += divvun-blanktag.1

if HAVE_CGSPELL
# divvun-cgspell binary:
divvun_cgspell_SOURCES  = main_cgspell.cpp cgspell.cpp
divvun_cgspell_LDADD    = $(HFSTOSPELL_LIBS)
divvun_cgspell_CXXFLAGS = $(HFSTOSPELL_CFLAGS)
bin_PROGRAMS           += divvun-cgspell
dist_man_MANS          += divvun-cgspell.1
endif

if HAVE_CHECKER
lib_LTLIBRARIES       = libdivvun.la
divvunincludedir      = $(includedir)/divvun
divvuninclude_HEADERS = checker.hpp checkertypes.hpp
libdivvun_la_SOURCES  = checker.cpp pipeline.cpp pipespec.cpp suggest.cpp blanktag.cpp normaliser.cpp
if HAVE_CGSPELL
libdivvun_la_SOURCES += cgspell.cpp
endif
# would've liked to put -lstdc++fs in LIBADD for <experimental/filesystem>, but macos
libdivvun_la_LIBADD   =              $(CG3_LIBS)   $(divvun_gen_sh_LDADD)    $(divvun_suggest_LDADD)    $(divvun_cgspell_LDADD)
libdivvun_la_CXXFLAGS =              $(CG3_CFLAGS) $(divvun_gen_sh_CXXFLAGS) $(divvun_suggest_CXXFLAGS) $(divvun_cgspell_CXXFLAGS)
libdivvun_la_LDFLAGS  = -no-undefined
# TODO something like -version-info 49:0:0

bin_PROGRAMS         += divvun-checker
dist_man_MANS        += divvun-checker.1

# divvun-checker binary:
divvun_checker_SOURCES  = main_checker.cpp
divvun_checker_LDADD    = libdivvun.la $(libdivvun_la_LIBADD)
divvun_checker_CXXFLAGS =              $(libdivvun_la_CXXFLAGS)
divvun_checkerdir       = $(datadir)
endif

# divvun-normaliser binary:
divvun_normaliser_SOURCES  = main_normaliser.cpp normaliser.cpp
divvun_normaliser_LDADD    = libdivvun.la $(libdivvun_la_LIBADD) $(HFST_LIBS)
divvun_normaliser_CXXFLAGS =              $(libdivvun_la_CXXFLAGS) $(HFST_CXXFLAGS)
bin_PROGRAMS            += divvun-normaliser
dist_man_MANS           += divvun-normaliser.1

# divvun-phon binary:
divvun_phon_SOURCES  = main_phon.cpp phon.cpp
divvun_phon_LDADD    = libdivvun.la $(libdivvun_la_LIBADD) $(HFST_LIBS)
divvun_phon_CXXFLAGS =              $(libdivvun_la_CXXFLAGS) $(HFST_CXXFLAGS)
bin_PROGRAMS            += divvun-phon
dist_man_MANS           += divvun-phon.1

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
