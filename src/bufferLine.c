#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"







void BufferLine_AppendLine(BufferLine* l, BufferLine* src) {
	printf("nyi bufferline_appendline\n");
}




void BufferLine_SetText(BufferLine* l, char* text, intptr_t len) {
	if(len == 0) {
		l->length = 0;
		return;
	};
	
	if(l->buf == NULL) {
		printf("null line buffer\n"); exit(1);
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


static void insert_line(Buffer* b, BufferLine* l) {
	l->id = b->nextLineID++;
	
	// resize the table if needed
	if(b->idTableFill >= b->idTableAlloc * .75) {
		
		size_t newAlloc = b->idTableAlloc * 2;
		size_t newMask = newAlloc - 1;
		
		BufferLine** new = calloc(1, sizeof(*new) * newAlloc);
		
		long f = 0;
		for(long i = 0; i < b->idTableAlloc; i++) {
			BufferLine* bl = b->idTable[i];
			if(!bl) continue;
			
			uint64_t hash = lineIDHash(bl->id) & newMask;
			for(long i = 0; i < newAlloc; i++) {
				if(new[hash] == 0) {
					new[hash] = bl;
					f++;
					if(f >= b->idTableFill) goto DONE;
					break;
				}
				
				hash = (hash + 1) & newMask;
			}
		}
		
	DONE:
		free(b->idTable);
		b->idTable = new;
		b->idTableAlloc = newAlloc;
		b->idTableMask = newMask;
	}
	
	uint64_t hash = lineIDHash(l->id) & b->idTableMask;
	for(long i = 0; i < b->idTableAlloc; i++) {
		if(b->idTable[hash] == 0) {
			b->idTable[hash] = l;
			b->idTableFill++;
			break;
		}
		
		hash = (hash + 1) & b->idTableMask;
	}
	
}


BufferLine* BufferLine_New(Buffer* b) {
	BufferLine* l = pcalloc(l);
	insert_line(b, l);
	
	l->allocSz = 32;
	l->buf = malloc(sizeof(*l->buf) * l->allocSz);
	l->flagBuf = malloc(sizeof(*l->flagBuf) * l->allocSz);
	l->buf[0] = 0;
	l->length = 0;
	return l;
}


void BufferLine_Delete(Buffer* b, BufferLine* l) {
	
	// remove it from the id hash table
	uint64_t hash = lineIDHash(l->id) & b->idTableMask;
	for(long i = 0; i < b->idTableAlloc; i++) {
		if(b->idTable[hash] == 0) break;
		if(b->idTable[hash]->id == l->id) {
			b->idTable[hash] = 0;
			b->idTableFill--;
			break;
		}
		
		hash = (hash + 1) & b->idTableMask;
	}
	
	
	if(l->buf) free(l->buf);
	if(l->flagBuf) {
		free(l->flagBuf);
	}
	VEC_FREE(&l->style);
	
	free(l);
}

BufferLine* BufferLine_FromStr(Buffer* b, char* text, intptr_t len) {
	BufferLine* l = BufferLine_New(b);
	BufferLine_SetText(l, text, len);
	return l;
}

BufferLine* BufferLine_Copy(Buffer* b, BufferLine* orig) {
	BufferLine* l = BufferLine_New(b);
	
	l->length = orig->length;
	l->allocSz = orig->allocSz;
	l->buf = realloc(l->buf, sizeof(*l->buf) * l->allocSz);
	l->flagBuf = realloc(l->flagBuf, sizeof(*l->flagBuf) * l->allocSz);
	l->flags = orig->flags;
	
	memcpy(l->buf, orig->buf, sizeof(*l->buf) * l->length);
	memcpy(l->flagBuf, orig->flagBuf, sizeof(*l->flagBuf) * l->length); // better than nothing
	VEC_COPY(&l->style, &orig->style);
	
	return l;
}


void BufferLine_EnsureAlloc(BufferLine* l, intptr_t len) {
	if(l->buf == NULL) {
		printf("null line buffer\n"); exit(1);
		l->allocSz = MAX(32ul, nextPOT(len + 1));
		l->buf = calloc(1, l->allocSz);
		l->flagBuf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, sizeof(*l->buf) * l->allocSz);
		l->flagBuf = realloc(l->flagBuf, sizeof(*l->flagBuf) * l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
	}
}



// does NOT handle embedded linebreak chars
void BufferLine_InsertChars(BufferLine* l, char* text, colnum_t col, intptr_t len) {
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
		intptr_t n = l->length - offset - len;
// 		n = MAX(0, n);
// 		printf("length: %ld, alloc: %ld, off: %ld, len: %ld, n: %ld\n", l->length, l->allocSz, offset, len, n);
		memmove(l->buf + offset, l->buf + offset + len, n);
	}
	
//	printf("delchar len:%ld, %ld\n", len, l->length);
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


void BufferLine_TruncateAfter(BufferLine* l, colnum_t col) {
	if(l->length < col) return;
	l->buf[col - 1] = 0;
	l->length = col - 1;
}

colnum_t BufferLine_GetIndentCol(BufferLine* l) {
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
	BufferLine* line = sel->line[0];
	
	while(line) {
		if(line == bl) return 1;
		
		if(line == sel->line[1]) break;
		line = line->next;
	}

	return 0;
}


