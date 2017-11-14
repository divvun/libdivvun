%.hfst: %.xfst
	hfst-regexp2fst --disjunct -i $< -o $@.tmp
	hfst-fst2fst -O -i $@.tmp -o $@


check_DATA = blanktagger.hfst
TESTS = ./run

CLEANFILES = $(check_DATA)

test: check
