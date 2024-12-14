#!/bin/bash

dir=`dirname $0`

cd $dir

gcc _build.c -lutil -o ._build -ggdb \
	&& ./._build $@ 
