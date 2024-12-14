#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -pd \
	&& gdb -x ~/.gdbinit -ex=r --args ./meshtool  $@ \
	&& gprof ./meshtool gmon.out > prof.out \
	&& nano prof.out
	
