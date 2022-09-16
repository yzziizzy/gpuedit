#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -d \
	&& ./valgrind ./gpuedit --config config/options.json $@
