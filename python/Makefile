all: libdivvun.py _libdivvun.so
	@echo
	@echo … done
	python3 -c 'import libdivvun'

%_wrap.c %_wrap.cxx %.py: %.i
	swig -Wall -c++ -python $<

#swig -Wall -c++ -python "-I$(HOME)/PREFIX/gtd-gc/include" $<

%_wrap.o: %_wrap.cxx
	g++ -c -fPIC libdivvun_wrap.cxx -I/usr/include/python3.6m

_%.so: %_wrap.o
	g++ -shared -Wl,-soname,$@ -o $@ ../src/.libs/libdivvun.so $<

clean:
	rm libdivvun.py _libdivvun.so libdivvun_wrap.o libdivvun_wrap.cxx
