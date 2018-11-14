bil.hfstol:
	printf '0\t1\tj\to\n1\t2\ta\tg\n2\t3\t+CC\t@0@\n3' | hfst-txt2fst --format=optimized-lookup-weighted -o $@

generator.hfst: generator.strings
	hfst-strings2fst -j -i $< -o $@

generator.hfstol: generator.hfst
	hfst-fst2fst -O -i $< -o $@


check_DATA = generator.hfstol
TESTS = ./run ./run-flushing ./run-genall ./validate

CLEANFILES = $(check_DATA) generator.hfst bil.hfstol

test: check
