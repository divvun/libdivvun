all: _libdivvun.cpython-36m-x86_64-linux-gnu.so libdivvun.py build/temp.linux-x86_64-3.6/libdivvun_wrap.o

_%.cpython-36m-x86_64-linux-gnu.so %.py build/temp.linux-x86_64-3.6/%_wrap.o: %.i ../src/.libs/%.so.0.0.0
	python3 setup.py build_ext --inplace --local-divvun


check:
	python3 -c 'import libdivvun; smegram = libdivvun.ArCheckerSpec("../test/checker/sme.zcheck").getChecker("smegram", True); print(smegram.proc_errs_bytes("seammas ballat ođđa dieđuiguin")[0].end);'

manual: libdivvun.py _libdivvun.so
	@echo
	@echo … done
	python3 -c 'import libdivvun'

%_wrap.c %_wrap.cxx %.py: %.i
	swig -Wall -c++ -python $<

#swig -Wall -c++ -python "-I$(HOME)/PREFIX/gtd-gc/include" $<

%_wrap.o: %_wrap.cxx
	g++ -c -fPIC libdivvun_wrap.cxx -I/usr/include/python3.6m

_%.so: %_wrap.o
	g++ -shared -Wl,-soname,$@ -o $@ "$(HOME)/PREFIX/gtd-gc/lib/libdivvun.so" $<

clean:
	rm -rf libdivvun.py _libdivvun.so libdivvun_wrap.o libdivvun_wrap.cxx build _libdivvun.cpython-36m-x86_64-linux-gnu.so libdivvun_wrap.cpp
