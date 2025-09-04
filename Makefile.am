# process this with automake to produce Makefile.in
ACLOCAL_AMFLAGS=-I m4

SUBDIRS = src test/suggest test/blanktag

if HAVE_CGSPELL
SUBDIRS += test/cgspell
endif

if HAVE_CHECKER
if HAVE_PYTHON_BINDINGS
SUBDIRS += python
endif # HAVE_PYTHON_BINDINGS
SUBDIRS += test/checker
endif # HAVE_CHECKER

test: check
