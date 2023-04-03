
EXTRA_DIST=run.xml run.archive run.spell run.workingdir run-lib run \
		   pipespec.xml tokeniser.pmscript analyser.lexc \
		   blanktagger.xfst \
		   valency.cg3 mwe-dis.cg3 disambiguator.cg3 grammarchecker.cg3 \
		   test-python-bindings.py \
		   input.xml.txt input.archive.txt input.spell.txt \
		   expected.xml.json expected.archive.json expected.spell.json

sme.zcheck: pipespec.xml tokeniser.pmhfst valency.cg3 mwe-dis.cg3 \
			disambiguator.cg3 grammarchecker.cg3 generator.hfstol \
			errors.xml acceptor.hfstol errmodel.hfst blanktagger.hfst
	rm -f $@
	zip -j $@ $^
	-cp $^ .

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

blanktagger.hfst: blanktagger.xfst
	hfst-regexp2fst --disjunct -i $< -o $@.tmp
	hfst-fst2fst -O -i $@.tmp -o $@
	rm $@.tmp

check_DATA=sme.zcheck tokeniser.pmhfst generator.hfstol errors.xml blanktagger.hfst

if HAVE_CGSPELL
TESTS=run.xml run.archive run.spell run.workingdir
if HAVE_PYTHON_BINDINGS
TESTS+=run-python-bindings
endif # HAVE_PYTHON_BINDINGS
# Keep the slowest one last:
TESTS+=run-lib
else
TESTS=run.nospell-xml run.nospell-archive
endif # HAVE_CGSPELL

CLEANFILES=sme.zcheck tokeniser.pmhfst generator.hfstol errors.xml \
		   blanktagger.hfst analyser.hfst generator.hfstol \
		   acceptor.hfstol errmodel.hfst \
		   grammarchecker.cg3 valency.cg3 mwe-dis.cg3 disambiguator.cg3 \
		   pipespec.xml \
		   output.spell.json output.archive.json output.xml.json \
		   output.workingdir.json
clean-local:
	rm -rf python-build

test: check
