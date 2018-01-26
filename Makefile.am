SUBDIRS = src test/suggest test/blanktag

if WANT_CGSPELL
SUBDIRS += test/cgspell
endif

if WANT_CHECKER
if WANT_PYTHON_BINDINGS
SUBDIRS += python
endif # WANT_PYTHON_BINDINGS
SUBDIRS += test/checker
endif # WANT_CHECKER

test: check
