#!/bin/bash


HL_CFLAGS="-I/usr/include/freetype2 -lm -std=gnu11 -g -DLINUX \
	-DSTI_C3DLAS_NO_CONFLICT \
	-DEACSMB_USE_SIMD \
	-DEACSMB_HAVE_SSE4 \
	-DEACSMB_HAVE_SSE41 \
	-DEACSMB_HAVE_AVX \
	-DEACSMB_HAVE_AVX2 \
	-msse4.1 -mavx -mavx2\
	-fno-math-errno \
	-fexcess-precision=fast \
	-fno-signed-zeros -fno-trapping-math -fassociative-math \
	-ffinite-math-only -fno-rounding-math \
	-fno-signaling-nans \
	-include signal.h \
	-pthread \
	-Wall \
	-Wno-unused-result \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-function \
	-Wno-pointer-sign \
	-Wno-missing-braces \
	-Wno-char-subscripts \
	-Wno-int-conversion \
	-Wno-int-to-pointer-cast \
	-Wno-unknown-pragmas \
	-Wno-sequence-point \
	-Wno-switch \
	-Werror-implicit-function-declaration \
	-Werror=uninitialized \
	-Werror=return-type"


gcc -shared -fPIC -o $1.so $1.c -lm $HL_CFLAGS



