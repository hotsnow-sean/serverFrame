.PHONY: xx

xx:
ifeq (build, $(wildcard build))
	cd build && make
else
	mkdir build
	cd build && cmake .. -G "MinGW Makefiles"
endif

%:
ifeq (build, $(wildcard build))
	cd build && make $@
else
	mkdir build
	cd build && cmake .. -G "MinGW Makefiles"
endif