
generator.hfst: generator.strings
	hfst-strings2fst -j -i $< -o $@

generator.hfstol: generator.hfst
	hfst-fst2fst -O -i $< -o $@


check_DATA = generator.hfstol
TESTS = ./run ./run-flushing ./validate

CLEANFILES = $(check_DATA) generator.hfst

test: check
