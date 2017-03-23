bin_PROGRAMS = divvun-suggest
bin_SCRIPTS = divvun-validate-suggest

divvun_suggest_SOURCES = main.cpp util.hpp json.hpp suggest.cpp suggest.hpp cxxopts.hpp

divvun_suggest_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
divvun_suggest_LDADD = $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS)


# Used by divvun-validate-suggest.in:
divvun_suggestdir = $(prefix)/share/divvun-suggest
divvun_suggest_DATA = errors.dtd errors.rnc
EXTRA_DIST = errors.dtd errors.rnc

if HAVE_TRANG
errors.rnc: errors.dtd
	test -f $(TRANG) && java -jar $(TRANG) -I dtd -O rnc $< /dev/stdout >$@.tmp
	@awk '{print}/^attlist/{print ""}' <$@.tmp >$@
	@rm -f $@.tmp
else
errors.rnc: errors.dtd
	@echo Could not find TRANG=$(TRANG), not updating errors.rnc
endif
