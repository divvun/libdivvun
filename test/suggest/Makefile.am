
EXTRA_DIST=generator.strings run run-flushing run-genall validate\
		   errors.xml  \
		   expected.addcohort-comma.err  \
		   expected.addcohort-comma.json  \
		   expected.addcohort-intervening.err  \
		   expected.addcohort-intervening.json  \
		   expected.addcohort-right.err  \
		   expected.addcohort-right.json  \
		   expected.badjel.err  \
		   expected.badjel.json  \
		   expected.blank-order.err  \
		   expected.blank-order.json  \
		   expected.casing.err  \
		   expected.casing.json  \
		   expected.controlchars.err  \
		   expected.controlchars.json  \
		   expected.defaults-id.err  \
		   expected.defaults-id.json  \
		   expected.defaults-re.err  \
		   expected.defaults-re.json  \
		   expected.delete-and-suggest.err  \
		   expected.delete-and-suggest.json  \
		   expected.delete-and-suggest-right.err  \
		   expected.delete-and-suggest-right.json  \
		   expected.delete.err  \
		   expected.delete.json  \
		   expected.delete-multiple2.err  \
		   expected.delete-multiple2.json  \
		   expected.delete-multiple.err  \
		   expected.delete-multiple.json  \
		   expected.delete-span.err  \
		   expected.delete-span.json  \
		   expected.dollar2.err  \
		   expected.dollar2.json  \
		   expected.fiinna.err  \
		   expected.fiinna.json  \
		   expected.flushing.err  \
		   expected.flushing.json  \
		   expected.generate-all.cg  \
		   expected.generate-all.err  \
		   expected.html-in-msg.err  \
		   expected.html-in-msg.json  \
		   expected.left-intervening.err  \
		   expected.left-intervening.json  \
		   expected.left-normspace.err  \
		   expected.left-normspace.json  \
		   expected.linebreaks.err  \
		   expected.linebreaks.json  \
		   expected.missinglink.err  \
		   expected.missinglink.json  \
		   expected.multiple-errtags.err  \
		   expected.multiple-errtags.json  \
		   expected.relations.err  \
		   expected.relations.json  \
		   expected.same-as-form.err  \
		   expected.same-as-form.json  \
		   expected.semroles.err  \
		   expected.semroles.json  \
		   expected.subreadings.err  \
		   expected.subreadings.json  \
		   expected.suggestwf-spaces.err  \
		   expected.suggestwf-spaces.json  \
		   expected.superblanks.err  \
		   expected.superblanks.json  \
		   expected.tabs.err  \
		   expected.tabs.json  \
		   expected.utf16.err  \
		   expected.utf16.json  \
		   expected.wfcasingright.err  \
		   expected.wfcasingright.json  \
		   input.addcohort-comma.cg  \
		   input.addcohort-intervening.cg  \
		   input.addcohort-right.cg  \
		   input.badjel.cg  \
		   input.blank-order.cg  \
		   input.casing.cg  \
		   input.controlchars.cg  \
		   input.defaults-id.cg  \
		   input.defaults-re.cg  \
		   input.delete-and-suggest.cg  \
		   input.delete-and-suggest-right.cg  \
		   input.delete.cg  \
		   input.delete-multiple2.cg  \
		   input.delete-multiple.cg  \
		   input.delete-span.cg  \
		   input.dollar2.cg  \
		   input.fiinna.cg  \
		   input.flushing.cg  \
		   input.generate-all.cg.genall  \
		   input.html-in-msg.cg  \
		   input.left-intervening.cg  \
		   input.left-normspace.cg  \
		   input.linebreaks.cg  \
		   input.missinglink.cg  \
		   input.multiple-errtags.cg  \
		   input.relations.cg  \
		   input.same-as-form.cg  \
		   input.semroles.cg  \
		   input.subreadings.cg  \
		   input.suggestwf-spaces.cg  \
		   input.superblanks.cg  \
		   input.tabs.cg  \
		   input.utf16.cg  \
		   input.wfcasingright.cg  \
		   invalid.xml

bil.hfstol:
	printf '0\t1\tj\to\n1\t2\ta\tg\n2\t3\t+CC\t@0@\n3' | hfst-txt2fst --format=optimized-lookup-weighted -o $@

generator.hfst: generator.strings
	hfst-strings2fst -j -i $< -o $@

generator.hfstol: generator.hfst
	hfst-fst2fst -O -i $< -o $@


check_DATA=generator.hfstol bil.hfstol
TESTS = run run-flushing run-genall validate

CLEANFILES=generator.hfst generator.hfstol bil.hfstol \
		   output.superblanks.json output.badjel.err \
		   output-flushing.blank-order.json output-flushing.tabs.json \
		   output-flushing.suggestwf-spaces.json \
		   output.delete-and-suggest.json output.blank-order.json \
		   output-flushing.delete-and-suggest.json \
		   output-flushing.casing.json output-flushing.subreadings.json \
		   output.subreadings.json output.delete-multiple.json \
		   output-flushing.linebreaks.json output.defaults-id.err \
		   output-flushing.addcohort-comma.json output.superblanks.err \
		   output-flushing.left-intervening.json \
		   output-flushing.flushing.json \
		   output.same-as-form.json output.utf16.json \
		   output.delete-span.json \
		   output.delete.err \
		   output.addcohort-intervening.err \
		   output.utf16.err \
		   output-flushing.superblanks.json \
		   output.relations.err \
		   output.dollar2.err \
		   output.left-normspace.json \
		   output-flushing.semroles.json \
		   output.flushing.err \
		   output.suggestwf-spaces.json \
		   output.delete-multiple2.json \
		   output-flushing.utf16.json \
		   output-flushing.left-normspace.json \
		   output.casing.err \
		   output.dollar2.json \
		   output.left-normspace.err \
		   output.subreadings.err \
		   output-flushing.addcohort-intervening.json \
		   output.html-in-msg.err \
		   output-flushing.addcohort-right.json \
		   output.delete-multiple2.err \
		   output.semroles.err \
		   output.defaults-re.json \
		   output-flushing.defaults-re.json \
		   output.semroles.json \
		   output.defaults-id.json \
		   output-flushing.delete.json \
		   output.wfcasingright.json \
		   output-flushing.delete-multiple.json \
		   output.missinglink.json \
		   output.addcohort-right.json \
		   output-flushing.same-as-form.json \
		   output-flushing.controlchars.json \
		   output.same-as-form.err \
		   output.delete.json \
		   output.casing.json \
		   output.controlchars.err \
		   output.delete-span.err \
		   output.fiinna.json \
		   output-flushing.missinglink.json \
		   output.fiinna.err \
		   output.suggestwf-spaces.err \
		   output.linebreaks.err \
		   output.addcohort-comma.json \
		   output.multiple-errtags.err \
		   output.linebreaks.json \
		   output.generate-all.cg \
		   output.left-intervening.err \
		   output.tabs.err \
		   output.delete-and-suggest.err \
		   output.left-intervening.json \
		   output.badjel.json \
		   output.addcohort-comma.err \
		   output-flushing.html-in-msg.json \
		   output.blank-order.err \
		   output-flushing.delete-multiple2.json \
		   output.relations.json \
		   output-flushing.defaults-id.json \
		   output.addcohort-intervening.json \
		   output.controlchars.json \
		   output.flushing.json \
		   output-flushing.multiple-errtags.json \
		   output.generate-all.err \
		   output.delete-and-suggest-right.err \
		   output.delete-multiple.err \
		   output-flushing.badjel.json \
		   output-flushing.relations.json \
		   output-flushing.fiinna.json \
		   output.tabs.json \
		   output.multiple-errtags.json \
		   output-flushing.dollar2.json \
		   output.addcohort-right.err \
		   output.missinglink.err \
		   output.html-in-msg.json \
		   output-flushing.delete-and-suggest-right.json \
		   output.defaults-re.err \
		   output-flushing.wfcasingright.json \
		   output-flushing.delete-span.json \
		   output.wfcasingright.err \
		   output.delete-and-suggest-right.json
test: check
