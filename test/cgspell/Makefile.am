%.hfstol: %.hfst
	hfst-fst2fst -w -i $< -o $@

%.invhfst: %.lexc
	hfst-lexc $< -o $@

%.hfst: %.invhfst
	hfst-invert -i $< -o $@

EXTRA_DIST=run.default run.X run.n2 run.skip run.flush run \
		   analyser.lexc \
		   errmodel.hfst \
		   expected.default \
		   expected.flush \
		   expected.n2 \
		   expected.skip \
		   expected.X \
		   input.default \
		   input.flush \
		   input.n2 \
		   input.skip \
		   input.X
check_DATA=analyser.hfstol errmodel.hfst
TESTS=run.default run.X run.n2 run.skip run.flush

CLEANFILES=analyser.hfst analyser.hfstol analyser.invhfst \
		   output.n2 output.X output.default output.flush output.skip

est: check
