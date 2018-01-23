SUBDIRS = src test/suggest test/blanktag

if WANT_CGSPELL
SUBDIRS += test/cgspell
endif

if WANT_CHECKER
SUBDIRS += test/checker
endif

if WANT_PYTHON_BINDINGS
SUBDIRS += python
endif

test: check
