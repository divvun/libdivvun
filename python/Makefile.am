SWIG_INTERFACE = libdivvun.i

EXTRA_DIST = libdivvun.i std_unique_ptr.i setup.py

BUILT_SOURCES = libdivvun_wrap.cpp libdivvun.py
CLEANFILES = $(BUILT_SOURCES)

all-local: $(BUILT_SOURCES)
	CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" $(PYTHON) setup.py build_ext

install-exec-local:
	$(PYTHON) setup.py install --prefix=$(DESTDIR)$(prefix)

uninstall-local:
	rm -f $(DESTDIR)$(libdir)/python*/site-packages/_libdivvun.*.so
	rm -f $(DESTDIR)$(libdir)/python*/site-packages/libdivvun.py
	rm -f $(DESTDIR)$(libdir)/python*/site-packages/__pycache__/libdivvun.*.pyc
	rm -f $(DESTDIR)$(libdir)/python*/site-packages/python_$(PACKAGE)-*.egg-info

clean-local:
	rm -rf build __pycache__

libdivvun_wrap.cpp libdivvun.py: $(SWIG_INTERFACE)
	$(SWIG) -c++ -python -I$(top_srcdir)/src/lib -o libdivvun_wrap.cpp -outdir . $(srcdir)/$(SWIG_INTERFACE)
