#!/bin/bash


../../sti/parser/parser_gen -gecndufsv js_tokens.txt 1> parser_generated.h


gcc  -shared -fPIC -o ../js.so \
	lexer.c ../../sti/sti.c \
	-lm -Wall -Werror -O0 -ggdb -DLINUX -std=gnu11 \
	-Wno-discarded-qualifiers \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-label



