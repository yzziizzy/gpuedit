#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"




BufferLine* Buffer_raw_GetLine(Buffer* b, intptr_t line) {
	if(line > b->numLines) return b->last;
	
	// TODO: faster algorithm
	
	BufferLine* bl = b->current;;
	if(line < b->current->lineNum) {
		while(bl->lineNum > line && bl->prev) bl = bl->prev; 
	}
	else if(line > b->current->lineNum) {
		while(bl->lineNum <= line - 1 && bl->next) bl = bl->next;
	}
	
	return bl;
}



void Buffer_raw_RenumberLines(BufferLine* start, intptr_t num) {
	if(!start) return;
	
	// renumber the rest of the lines
	BufferLine* q = start;
	q->lineNum = num++;
	while(q) {
		q->lineNum = num++;
		q = q->next;
	}
}


BufferLine* Buffer_raw_InsertLineAfter(Buffer* b, BufferLine* before) {
	
	if(before == NULL && b->first) {
		printf("before is null in InsertLineAfter\n");
		return NULL;
	}
	
	b->numLines++;
	BufferLine* after = BufferLine_New();
		
	if(b->first == NULL) {
		b->first = after;
		b->last = after;
		b->current = after;
		after->lineNum = 1;
		return after;
	}
	
	after->next = before->next;
	after->prev = before;
	before->next = after;
	if(after->next) after->next->prev = after;
	
	if(before == b->last) {
		b->last = after;
	}
	
	Buffer_raw_RenumberLines(after, before->lineNum);
	
	return after;
};




BufferLine* Buffer_raw_InsertLineBefore(Buffer* b, BufferLine* after) {
	
	if(after == NULL && b->first) {
		printf("after is null in InsertLineBefore\n");
		return NULL;
	}
	
	b->numLines++;
	BufferLine* before = BufferLine_New();
	
	if(b->first == NULL) {
		b->first = before;
		b->last = before;
		b->current = before;
		before->lineNum = 1;
		return before;
	}
	
	before->next = after;
	before->prev = after->prev;
	after->prev = before;
	if(before->prev) before->prev->next = before;
	
	if(after == b->first) {
		b->first = before;
	}
	
	Buffer_raw_RenumberLines(before, after->lineNum - 1);
	
	return before;
};




void Buffer_raw_DeleteLine(Buffer* b, BufferLine* bl) {
	if(bl == NULL) return;
	
	if(bl == b->first) b->first = bl->next;
	if(bl == b->last) b->last = bl->prev;
	
	if(bl == b->current) {
		if(bl->next) b->current = bl->next;
		else if(bl->prev) b->current = bl->prev;
		else {
			printf("current line set to null in DeleteLine\n");
			b->current = NULL;
		}
	}

	if(bl->next) bl->next->prev = bl->prev;
	if(bl->prev) bl->prev->next = bl->next;
	
	// TODO check selections
	// TODO renumber lines 
	Buffer_raw_RenumberLines(bl->next, bl->lineNum - 1);
	
	if(b->useDict) Buffer_RemoveLineFromDict(b, bl);
	
	BufferLine_Delete(bl);
	
	b->numLines--;
};




void Buffer_raw_InsertChars(Buffer* b, BufferLine* bl, char* txt, intptr_t offset, intptr_t len) {
	if(b->useDict) Buffer_RemoveLineFromDict(b, bl);
	BufferLine_InsertChars(bl, txt, offset, len);
	if(b->useDict) Buffer_AddLineToDict(b, bl);
};



void Buffer_raw_DeleteChars(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len) {
	if(b->useDict) Buffer_RemoveLineFromDict(b, bl);
	BufferLine_DeleteChars(bl, offset, len);
	if(b->useDict) Buffer_AddLineToDict(b, bl);
};

