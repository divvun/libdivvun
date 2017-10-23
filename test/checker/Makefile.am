sme.zcheck: pipespec.xml tokeniser.pmhfst valency.cg3 mwe-dis.cg3 disambiguator.cg3 grammarchecker.cg3 generator.hfstol errors.xml acceptor.hfstol errmodel.hfst
	rm -f $@
	zip $@ $^

tokeniser.pmhfst: tokeniser.pmscript analyser.hfst
	hfst-pmatch2fst <$< >$@.tmp
	mv $@.tmp $@

analyser.hfst: analyser.lexc
	hfst-lexc $< -o inv.$@
	hfst-invert -i inv.$@ -o $@
	@rm -f inv.$@

acceptor.hfstol: analyser.hfst
	hfst-fst2fst -w -i $< -o $@

generator.hfstol: ../suggest/generator.hfstol
	cp $< $@

errors.xml: ../suggest/errors.xml
	cp $< $@

errmodel.hfst: ../cgspell/errmodel.hfst
	cp $< $@

check_DATA = sme.zcheck tokeniser.pmhfst generator.hfstol errors.xml
TESTS = ./run.xml.sh ./run.archive.sh ./run.spell.sh ./run-lib

CLEANFILES = $(check_DATA) analyser.hfst generator.hfstol errors.xml acceptor.hfstol errmodel.hfst

test: check
