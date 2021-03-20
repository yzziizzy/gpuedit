#!/bin/bash


../../sti/parser/parser_gen -gecndufsv lua_tokens.txt 1> parser_generated.h


OPTS="-fdata-sections -ffunction-sections -fPIC \
	-lm -Wall -Werror -O3 -ggdb -DLINUX -std=gnu11 \
	-Wno-discarded-qualifiers \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-label \
	-Wno-strict-aliasing"


gcc $OPTS -c lexer.c -o lexer.o
gcc $OPTS -c ../../sti/vec.c -o vec.o

ar rcs lua.a *.o

gcc -shared -Wl,--gc-sections,-u,gpuedit_list_highlighters lua.a -o ../lua.so



