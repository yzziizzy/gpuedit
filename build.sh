#!/bin/bash

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build \
	&& ./gpuedit --config config/options.json $@
