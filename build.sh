#!/bin/bash

gcc -lutil _build.c -o ._build -ggdb \
	&& ./._build \
	&& ./gpuedit --config config/options.json $@
