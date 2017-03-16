bin_PROGRAMS = divvun-suggest
bin_SCRIPTS = divvun-validate-suggest

divvun_suggest_SOURCES = main.cpp util.hpp json.hpp suggest.cpp suggest.hpp cxxopts.h

divvun_suggestdir = $(prefix)/share/divvun-suggest
divvun_suggest_DATA = errors.dtd errors.rnc


divvun_suggest_CXXFLAGS = $(HFSTOSPELL_CFLAGS) $(HFST_CFLAGS) $(PUGIXML_CFLAGS)
divvun_suggest_LDADD = $(HFSTOSPELL_LIBS) $(HFST_LIBS) $(PUGIXML_LIBS)

divvun-validate-suggest: Makefile.am validate-header.sh ../config.status
	@echo "Creating divvun-validate-suggest script"
	@echo "#!$(BASH)" > $@
	@cat validate-header.sh >> $@
	@echo "$(XMLLINT) --dtdvalid \"$(divvun_suggestdir)\"/errors.dtd --noout \"\$$FILE1\"" >>$@
	@chmod a+x $@

TRANG ?= $(HOME)/.jedit/jars/trang.jar

errors.rnc: errors.dtd
	java -jar $(TRANG) -I dtd -O rnc $< /dev/stdout >$@.tmp
	@awk '{print}/^attlist/{print ""}' <$@.tmp >$@
	@rm -f $@.tmp
