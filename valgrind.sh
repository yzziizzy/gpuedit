#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -d \
	&& ./valgrind ./gpuedit  -n --no-sessions --config config/options.json $@
