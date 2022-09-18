

#include "buffer.h"






int BufferRange_CompleteLinesOnly(BufferRange* sel) {
	if(!sel) return 0;
	
	if(sel->col[0] != 0 && sel->col[0] != sel->line[0]->length) return 0;
	if(sel->col[1] != 0 && sel->col[1] != sel->line[1]->length) return 0;
	
	return 1;
}

void BufferRange_DeleteLineNotify(BufferRange* r, BufferRange* dsel) {
	if(BufferLine_IsInRange(r->line[0], dsel)) {
		r->line[0] = dsel->line[0]->prev;
		if(!r->line[0]) {		
			r->line[0] = dsel->line[1]->next;		
		}
	}

	if(BufferLine_IsInRange(r->line[1], dsel)) {
		r->line[1] = dsel->line[0]->prev;
		if(!r->line[1]) {		
			r->line[1] = dsel->line[1]->next;		
		}
	}

}

	
void BufferRange_MoveMarkerH(BufferRange* r, int c, colnum_t cols) {
	// TODO: undo
	
	Buffer_RelPosH(r->line[c], r->col[c], cols, &r->line[c], &r->col[c]);
	r->colWanted = r->col[c];
	
	BufferRange_Normalize(r);
}

void BufferRange_MoveCursorH(BufferRange* r, colnum_t cols) {
	BufferRange_MoveMarkerH(r, r->cursor, cols);
}
void BufferRange_MovePivotH(BufferRange* r, colnum_t cols) {
	BufferRange_MoveMarkerH(r, !r->cursor, cols);
}
void BufferRange_MoveStartH(BufferRange* r, colnum_t cols) {
	BufferRange_MoveMarkerH(r, 0, cols);
}
void BufferRange_MoveEndH(BufferRange* r, colnum_t cols) {
	BufferRange_MoveMarkerH(r, 1, cols);
}


void BufferRange_MoveMarkerV(BufferRange* r, int c, linenum_t lines) {
	// TODO: undo
	
	Buffer_RelPosV(r->line[c], r->col[c], lines, &r->line[c], &r->col[c]);
	BufferRange_Normalize(r);
}

void BufferRange_MoveCursorV(BufferRange* r, linenum_t lines) {
	BufferRange_MoveMarkerV(r, r->cursor, lines);
}
void BufferRange_MovePivotV(BufferRange* r, linenum_t lines) {
	BufferRange_MoveMarkerV(r, !r->cursor, lines);
}
void BufferRange_MoveStartV(BufferRange* r, linenum_t lines) {
	BufferRange_MoveMarkerV(r, 0, lines);
}
void BufferRange_MoveEndV(BufferRange* r, linenum_t lines) {
	BufferRange_MoveMarkerV(r, 1, lines);
}


BufferRange* BufferRange_New(GUIBufferEditControl* w) {
	BufferRange* r = pcalloc(r);
	r->col[1] = -1;
	r->line[1] = 0;
	
	return r;
}

// make sure a range goes in the right direction
int BufferRange_Normalize(BufferRange* r) {
	if(!r) return 0;
	if(!r->line[0] || !r->line[1]) return 0;
	
	if(r->line[0] == r->line[1]) {
		if(r->col[0] == r->col[1]) {
			// doesn't matter.
		}
		else if(r->col[0] > r->col[1]) {
			intptr_t t = r->col[0];
			r->col[0] = r->col[1];
			r->col[1] = t;
			r->cursor = !r->cursor; // flip the cursor/pivot flag
		}
	}
	else if(r->line[0]->lineNum > r->line[1]->lineNum) {
		void* tl = r->line[0];
		r->line[0] = r->line[1];
		r->line[1] = tl;
		intptr_t t = r->col[0];
		r->col[0] = r->col[1];
		r->col[1] = t;
		
		r->cursor = !r->cursor; // flip the cursor/pivot flag
	}
	
// 	printf("sel: %d:%d -> %d:%d\n", br->line[0]->lineNum, br->col[0], br->line[1]->lineNum, br->col[1]);
	return 0;
}


// returns 1 for inside, 0 for outside
int BufferRangeSet_test(BufferRangeSet* s, BufferLine* bl, colnum_t col) {
	if(!s) return 0;
	
	VEC_EACH(&s->ranges, i, r) {
		if(!r->line[0] || !r->line[1]) continue;
		
		if(r->line[0]->lineNum > bl->lineNum) continue;
		if(r->line[1]->lineNum < bl->lineNum) continue; // TODO possible optimization
		
		// within the line range
		if(r->line[0] == bl && r->col[0] > col) continue;
		if(r->line[1] == bl && r->col[1] <= col) continue;
		
		
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
		if(r->line[1]->lineNum > bl->lineNum) continue;
		if(r->line[0]->lineNum >= bl->lineNum) {
			return i;
		}
	}
	
	return 0;
}






