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




void BufferLine_SetText(BufferLine* l, char* text, intptr_t len) {
	if(len == 0) {
		l->length = 0;
		return;
	};
	
	if(l->buf == NULL) {
		l->allocSz = nextPOT(len + 1);
		l->buf = calloc(1, l->allocSz);
		l->flagBuf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		l->flagBuf = realloc(l->flagBuf, l->allocSz);
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
	if(l->flagBuf) free(l->flagBuf);
	VEC_FREE(&l->style);
}

BufferLine* BufferLine_FromStr(char* text, intptr_t len) {
	BufferLine* l = BufferLine_New();
	BufferLine_SetText(l, text, len);
	return l;
}

BufferLine* BufferLine_Copy(BufferLine* orig) {
	BufferLine* l = BufferLine_New();
	l->length = orig->length;
	l->allocSz = orig->allocSz;
	l->buf = calloc(1, l->allocSz);
	l->flagBuf = calloc(1, l->allocSz);
	l->flags = orig->flags;
	strncpy(l->buf, orig->buf, l->length);
	strncpy(l->flagBuf, orig->flagBuf, l->length); // better than nothing
	VEC_COPY(&l->style, &orig->style);
	return l;
}


void BufferLine_EnsureAlloc(BufferLine* l, intptr_t len) {
	if(l->buf == NULL) {
		l->allocSz = MAX(32, nextPOT(len + 1));
		l->buf = calloc(1, l->allocSz);
		l->flagBuf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		l->flagBuf = realloc(l->flagBuf, l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
	}
}



// does NOT handle embedded linebreak chars
void BufferLine_InsertChars(BufferLine* l, char* text, intptr_t col, intptr_t len) {
	if(text == NULL) return;
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	if(col < l->length) {
		memmove(l->buf + col + len, l->buf + col, l->length - col);
	}
	
	memcpy(l->buf + col, text, len);
	
	l->length += len;
	l->buf[l->length] = 0; // just in case
}



void BufferLine_DeleteChars(BufferLine* l, intptr_t offset, intptr_t len) {
	if(l->length == 0) return;
	if(offset > l->length + 1) return; // strange overrun
	
	if(offset < l->length) {
		intptr_t n = l->length - offset - len + 1; // +1 for the null terminator
// 		n = MAX(0, n);
// 		printf("length: %ld, alloc: %ld, off: %ld, len: %ld, n: %ld\n", l->length, l->allocSz, offset, len, n);
		memmove(l->buf + offset, l->buf + offset + len, n);
	}
	
	l->length -= len;
	l->buf[l->length] = 0;
}


// does NOT handle embedded linebreak chars
void BufferLine_AppendText(BufferLine* l, char* text, intptr_t len) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	memcpy(l->buf + l->length, text, len);
	
	l->length += len;
}


void BufferLine_TruncateAfter(BufferLine* l, intptr_t col) {
	if(l->length < col) return;
	l->buf[col - 1] = 0;
	l->length = col - 1;
}

intptr_t BufferLine_GetIndentCol(BufferLine* l) {
	intptr_t i = 0;
	for(; i < l->length; i++) {
		switch(l->buf[i]) {
			case ' ':
			case '\t':
				continue;
		}
		break;
	}
	return i;
}


int BufferLine_IsInRange(BufferLine* bl, BufferRange* sel) {
	BufferLine* line = sel->startLine;
	
	while(line) {
		if(line == bl) return 1;
		
		if(line == sel->endLine) break;
		line = line->next;
	}

	return 0;
}


