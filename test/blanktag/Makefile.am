SUFFIXES=.hfst .xfst

.xfst.hfst:
	hfst-regexp2fst --disjunct -i $< -o $@.tmp
	hfst-fst2fst -O -i $@.tmp -o $@
	rm $@.tmp

EXTRA_DIST=run \
		   blanktagger.xfst \
		   expected.cg \
		   expected.ends.cg \
		   input.cg \
		   input.ends.cg
check_DATA=blanktagger.hfst
TESTS=run

CLEANFILES=blanktagger.hfst \
		   output.cg output.ends.cg

test: check
