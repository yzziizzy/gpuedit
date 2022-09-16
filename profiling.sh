#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -pd \
	&& gdb -x ~/.gdbinit -ex=r --args ./gpuedit --config config/options.json $@ \
	&& gprof ./gpuedit gmon.out > prof.out \
	&& nano prof.out
	
