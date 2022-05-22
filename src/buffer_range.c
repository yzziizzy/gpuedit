

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

void BufferRange_MoveCursorH(BufferRange* r, colnum_t cols) {
	int i = cols;
	// TODO: undo
	
	if(i < 0) while(i++ < 0) {
		if(r->col[0] <= 0) {
			
			if(r->line[0]->prev == NULL) {
				r->col[0] = 0;
				break;
			}
			
			r->line[0] = r->line[0]->prev;
			r->col[0] = r->line[0]->length;
		}
		else {
			r->col[0]--;
		}
	}
	else while(i-- > 0) {
		if(r->col[0] >= r->line[0]->length) {
			
			if(r->line[0]->next == NULL) {
				break;
			}
			
			r->line[0] = r->line[0]->next;
			r->col[0] = 0;
		}
		else {
			r->col[0]++;
		}
	}
	
	r->colWanted = r->col[0];
}


BufferRange* BufferRange_New(GUIBufferEditControl* w) {
	BufferRange* r = pcalloc(r);
	r->col[1] = -1;
	r->line[1] = 0;
	
	return r;
}

// make sure a range goes in the right direction
// delete selections that are zero
// returns 0 if the selection is still valid, 1 if it was deleted
int BufferRange_Normalize(BufferRange** pbr) {
	BufferRange* br = *pbr;
	if(!br) return 0;
	
	if(br->line[0] == br->line[1]) {
		if(br->col[0] == br->col[1]) {
			free(br); // delete empty ranges
			*pbr = NULL;
			return 1;
		}
		else if(br->col[0] > br->col[1]) {
			intptr_t t = br->col[0];
			br->col[0] = br->col[1];
			br->col[1] = t;
		}
	}
	else if(br->line[0]->lineNum > br->line[1]->lineNum) {
		void* tl = br->line[0];
		br->line[0] = br->line[1];
		br->line[1] = tl;
		intptr_t t = br->col[0];
		br->col[0] = br->col[1];
		br->col[1] = t;
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






