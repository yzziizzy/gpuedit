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







