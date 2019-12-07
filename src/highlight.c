#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "highlight.h"
#include "common_math.h"
#include "common_gl.h"


 
#define CHECK_ALLOC(buf, allocsz, len, inc, extra) \
do { \
	if((buf) == NULL) { \
		(allocsz) = MAX(32, nextPOT((inc) + (extra))); \
		(buf) = malloc(sizeof(*(buf)) * (allocsz)); \
	} \
	else if((allocsz) < (len) + (inc) + (extra)) { \
		(allocsz) = nextPOT((len) + (inc) + (extra));  \
		(buf) = realloc((buf), sizeof(*(buf)) * (allocsz)); \
	} \
} while(0);


/*
void TextStyleMeta_push(TextStyleMeta* meta, int index, size_t len) {
	CHECK_ALLOC(meta->src, meta->allocSz, meta->length, 1, 0);
	meta->src[meta->length].styleIndex = index;
	meta->src[meta->length].length = len;
	meta->length++;
}



void HL_acceptLine(char* text, size_t len, TextStyleMeta* meta) {
	
	for(int i = 0; i < len; i++) {
		TextStyleMeta_push(meta, i % 2 + 1, 1);
	}
	
}

*/




