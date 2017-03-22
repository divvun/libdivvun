SUBDIRS = src

test/generator.hfst: test/generator.strings
	hfst-strings2fst -j -i $< -o $@

test/generator.hfstol: test/generator.hfst
	hfst-fst2fst -O -i $< -o $@

check_DATA=test/generator.hfstol
TESTS=test/run test/run-flushing test/validate

test: check
