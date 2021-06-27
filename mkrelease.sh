#!/bin/sh

sudo touch $0


./autogen.sh CFLAGS='-O3' $* \
&& make clean \
&& make -j \
&& sudo cp src/gpuedit /usr/bin/ \
&& ./autogen.sh CFLAGS='-O0' \
&& make clean \
&& make -j
