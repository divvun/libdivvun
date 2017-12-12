all: libdivvun.py
	@echo
	@echo … done

%_wrap.c %_wrap.cxx %.py: %.i
	swig -Wall -c++ -python $<

#swig -Wall -c++ -python "-I$(HOME)/PREFIX/gtd-gc/include" $<

