

#include "buffer.h"






int BufferRange_CompleteLinesOnly(BufferRange* sel) {
	if(!sel) return 0;
	
	if(sel->startCol != 0 && sel->startCol != sel->startLine->length) return 0;
	if(sel->endCol != 0 && sel->endCol != sel->endLine->length) return 0;
	
	return 1;
}

void BufferRange_DeleteLineNotify(BufferRange* r, BufferRange* dsel) {
	if(BufferLine_IsInRange(r->startLine, dsel)) {
		r->startLine = dsel->startLine->prev;
		if(!r->startLine) {		
			r->startLine = dsel->endLine->next;		
		}
	}

	if(BufferLine_IsInRange(r->endLine, dsel)) {
		r->endLine = dsel->startLine->prev;
		if(!r->endLine) {		
			r->endLine = dsel->endLine->next;		
		}
	}

}

BufferRange* BufferRange_New(GUIBufferEditControl* w) {
	BufferRange* r = pcalloc(r);
	r->endCol = -1;
	r->endLine = 0;
	
	return r;
}

// make sure a range goes in the right direction
// delete selections that are zero
// returns 0 if the selection is still valid, 1 if it was deleted
int BufferRange_Normalize(BufferRange** pbr) {
	BufferRange* br = *pbr;
	if(!br) return 0;
	
	if(br->startLine == br->endLine) {
		if(br->startCol == br->endCol) {
			free(br); // delete empty ranges
			*pbr = NULL;
			return 1;
		}
		else if(br->startCol > br->endCol) {
			intptr_t t = br->startCol;
			br->startCol = br->endCol;
			br->endCol = t;
		}
	}
	else if(br->startLine->lineNum > br->endLine->lineNum) {
		void* tl = br->startLine;
		br->startLine = br->endLine;
		br->endLine = tl;
		intptr_t t = br->startCol;
		br->startCol = br->endCol;
		br->endCol = t;
	}
	
// 	printf("sel: %d:%d -> %d:%d\n", br->startLine->lineNum, br->startCol, br->endLine->lineNum, br->endCol);
	return 0;
}


// returns 1 for inside, 0 for outside
int BufferRangeSet_test(BufferRangeSet* s, BufferLine* bl, colnum_t col) {
	if(!s) return 0;
	
	VEC_EACH(&s->ranges, i, r) {
		if(!r->startLine || !r->endLine) continue;
		
		if(r->startLine->lineNum > bl->lineNum) continue;
		if(r->endLine->lineNum < bl->lineNum) continue; // TODO possible optimization
		
		// within the line range
		if(r->startLine == bl && r->startCol > col) continue;
		if(r->endLine == bl && r->endCol <= col) continue;
		
		return 1;
	}
	
	return 0;
}


void BufferRangeSet_FreeAll(BufferRangeSet* s) {
	if(!s) return;
	
	VEC_EACH(&s->ranges, i, r) {
		if(r) free(r);
	}
	VEC_FREE(&s->ranges);
	VEC_LEN(&s->ranges) = 0;
}


long BufferRange_FindNextRangeSet(BufferRangeSet* rs, BufferLine* line, colnum_t col) {
	BufferLine* bl = line;
	intptr_t c = col;
	
	if(!rs) return -1;
	
	if(!VEC_LEN(&rs->ranges)) return -1;
	
	VEC_EACH(&rs->ranges, i, r) {
		if(r->endLine->lineNum > bl->lineNum) continue;
		if(r->startLine->lineNum >= bl->lineNum) {
			return i;
		}
	}
	
	return 0;
}






