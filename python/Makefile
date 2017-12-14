all: libdivvun.py _libdivvun.cpython-36m-x86_64-linux-gnu.so build/temp.linux-x86_64-3.6/libdivvun_wrap.o

_%.cpython-36m-x86_64-linux-gnu.so %.py build/temp.linux-x86_64-3.6/%_wrap.o: %.i ../src/.libs/%.so.0.0.0
	python3 setup.py build_ext --inplace --local-divvun


check:
	python3 test.py

clean:
	rm -rf libdivvun.py _libdivvun.so libdivvun_wrap.o build _libdivvun.cpython-36m-x86_64-linux-gnu.so libdivvun_wrap.cpp
