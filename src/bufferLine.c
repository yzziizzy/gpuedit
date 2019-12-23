#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"






void BufferLine_AppendLine(BufferLine* l, BufferLine* src) {
	printf("nyi bufferline_appendline\n");
}




void BufferLine_SetText(BufferLine* l, char* text, size_t len) {
	if(len == 0) {
		l->length = 0;
		return;
	};
	
	if(l->buf == NULL) {
		l->allocSz = nextPOT(len + 1);
		l->buf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
	
	strncpy(l->buf, text, len);
	l->length = len;
}



BufferLine* BufferLine_New() {
	BufferLine* l = pcalloc(l);
	return l;
}

void BufferLine_Delete(BufferLine* l) {
	if(l->buf) free(l->buf);
	VEC_FREE(&l->style);
}

BufferLine* BufferLine_FromStr(char* text, size_t len) {
	BufferLine* l = BufferLine_New();
	BufferLine_SetText(l, text, len);
	return l;
}

BufferLine* BufferLine_Copy(BufferLine* orig) {
	BufferLine* l = BufferLine_New();
	l->length = orig->length;
	l->allocSz = orig->allocSz;
	l->buf = calloc(1, l->allocSz);
	l->flags = orig->flags;
	strncpy(l->buf, orig->buf, l->length);
	VEC_COPY(&l->style, &orig->style);
	return l;
}


void BufferLine_EnsureAlloc(BufferLine* l, size_t len) {
	if(l->buf == NULL) {
		l->allocSz = MAX(32, nextPOT(len + 1));
		l->buf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
}



// does NOT handle embedded linebreak chars
void BufferLine_InsertChars(BufferLine* l, char* text, size_t col, size_t len) {
	if(text == NULL) return;
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	if(col - 1 < l->length) {
		memmove(l->buf + col - 1 + len, l->buf + col - 1, l->length - col);
	}
	
	memcpy(l->buf + col, text, len);
	
	l->length += len;
	l->buf[l->length] = 0; // just in case
}



void BufferLine_DeleteChars(BufferLine* l, size_t offset, size_t len) {
	if(l->length == 0) return;
	if(offset > l->length + 2) return; // strange overrun
	
	if(offset - 2 < l->length) {
		memmove(l->buf + offset - 2, l->buf + offset - 1, l->length - offset + 3);
	}
	
	l->length -= 1;
}


// does NOT handle embedded linebreak chars
void BufferLine_AppendText(BufferLine* l, char* text, size_t len) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	memcpy(l->buf + l->length, text, len);
	
	l->length += len;
}


void BufferLine_TruncateAfter(BufferLine* l, size_t col) {
	if(l->length < col) return;
	l->buf[col - 1] = 0;
	l->length = col - 1;
}

void BufferLine_DeleteRange(BufferLine* l, size_t startC, size_t endC) {
	
	assert(startC > 0);
	assert(endC > 0);
	
	startC = MIN(MIN(endC, startC), l->length + 1);
	endC = MIN(MAX(endC, startC), l->length + 1);
	
	if(startC == endC) return;
	
	memmove(l->buf + startC - 1, l->buf + endC, l->length - endC);
	
	
	l->length -= endC - startC + 1;
	l->buf[l->length] = 0;
}
