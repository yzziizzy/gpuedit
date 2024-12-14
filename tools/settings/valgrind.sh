#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build -d \
	&& valgrind --error-limit=no --track-origins=yes ./gen_opts_structs $@
