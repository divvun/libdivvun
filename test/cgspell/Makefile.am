%.hfstol: %.hfst
	hfst-fst2fst -w -i $< -o $@

%.invhfst: %.lexc
	hfst-lexc $< -o $@

%.hfst: %.invhfst
	hfst-invert -i $< -o $@

check_DATA = analyser.hfstol
TESTS = ./run.default ./run.X ./run.n2 ./run.skip

CLEANFILES = $(check_DATA) analyser.hfst analyser.invhfst

test: check
