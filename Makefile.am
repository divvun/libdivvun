SUBDIRS = src

test/generator.hfst: test/generator.strings
	hfst-strings2fst -i $< -o $@

test/generator.hfstol: test/generator.hfst
	hfst-fst2fst -O -i $< -o $@

check: test/generator.hfstol
	@test/run
	@echo "All tests pass"

test: check
