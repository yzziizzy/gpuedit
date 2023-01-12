#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -d \
	&& gdb -x ~/.gdbinit -ex=r --args ./gpuedit -n --no-sessions --config config/options.json $@
