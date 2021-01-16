
#include "stdlib.h"
#include "string.h"




#include "gui.h"
#include "gui_internal.h"





static GUIHeader* hitTest(GUIGridLayout* gl, Vector2 absTestPos);
static void updatePos(GUIGridLayout* gl, GUIRenderParams* grp, PassFrameParams* pfp);




GUIGridLayout* GUIGridLayout_new(GUIManager* gm, Vector2 pos, Vector2 spacing) {
	
	GUIGridLayout* w;
	
	static struct gui_vtbl static_vt = {
		.UpdatePos = (void*)updatePos,
		.HitTest = (void*)hitTest,
	};
	
	
	w = calloc(1, sizeof(*w));
	CHECK_OOM(w);
	
	gui_headerInit(&w->header, gm, &static_vt, NULL);
	
	w->spacing = spacing;
	
	w->maxRows = -1;
	w->maxCols = -1;
	
	w->header.topleft = pos;
	
	return w;
}



// TODO: re-sort








static void updatePos(GUIGridLayout* gl, GUIRenderParams* grp, PassFrameParams* pfp) {
	int row;
	int col;
	
	// TODO: fix fencepost issue with spacing
	// TODO: figure out better phase for size calculation
// 	float total_h = 0.0;
// 	float max_w = 0.0;
// 	VEC_EACH(&cl->header.children, i, child) { 
// 		total_h += cl->spacing + child->h.size.y;
// 		max_w = fmax(max_w, child->h.size.x);
// 	}
	
	// gridLayout fills rows left to right, then top to bottom
	
	// if there are no column or row limits, limit to the parent's size
	
	
	int actualRows = MIN(gl->maxRows, ceil((float)VEC_LEN(&gl->header.children) / gl->maxCols));
	
	gl->header.size.y = actualRows * gl->spacing.y;
	gl->header.size.x = gl->maxCols * gl->spacing.x;
	
	Vector2 tl = gui_calcPosGrav(&gl->header, grp);
	
	gl->header.absTopLeft = tl;
	
	
	// gridLayout works by spoofing the renderparams supplied to each child
	row = 0;
	col = 0;
	VEC_EACH(&gl->header.children, i, child) { 
		
		GUIRenderParams grp2 = {
			.clip = grp->clip,
			.size = child->size, // sized to the child to eliminate gravity 
			.offset = {
				.x = tl.x + (col * gl->spacing.x),
				.y = tl.y + (row * gl->spacing.y)
			}
		};
		
		GUIHeader_updatePos(child, &grp2, pfp);
		
		col++;
		if(col >= gl->maxCols) {
			row++;
			col = 0;
			
			if(row >= gl->maxRows) {
				break;
			}
		}
		
	}
}



static GUIHeader* hitTest(GUIGridLayout* gl, Vector2 absTestPos) {
	GUIHeader* h = &gl->header;
	
	size_t i;
	GUIHeader* bestKid = NULL;
	for(i = 0; i < VEC_LEN(&h->children); i++) {
		GUIHeader* kid = GUIHeader_hitTest(VEC_ITEM(&h->children, i), absTestPos);
		if(kid) {
			if(!bestKid) {
				bestKid = kid;
			}
			else {
				if(kid->absZ > bestKid->absZ) bestKid = kid;
			}
		}
	}
	
// 	printf("hit: %p, %p\n", h, bestKid);
	return bestKid;
}


