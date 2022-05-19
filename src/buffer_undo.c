




#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"






char* undo_actions_names[] = {
#define X(a) [UndoAction_##a] = #a,
	UNDO_ACTION_LIST
#undef X
	NULL,
};





#include "ui/macros_on.h"

void BufferUndo_DebugRender(GUIManager* gm, Buffer* w, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	if(!gm->drawMode) return;

	gm->curZ += 1000000;
	
	for(int j = 0; j < w->undoFill; j++) {
		int i = (w->undoOldest + j) % w->undoMax; 
	
		BufferUndo_DebugRenderItem(gm, &w->undoRing[i], V(tl.x, tl.y + i * 16), sz, pfp);
	}
	
	// actual undo system indices
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 32), "Courier New", 12, &C4H(7777ffff), "Oldest  %d", w->undoOldest);
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 16), "Courier New", 12, &C4H(00ff00ff), "Current %d", w->undoCurrent);
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 48), "Courier New", 12, &C4H(ff0000ff), "Newest  %d", w->undoNewest);
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 64), "Courier New", 12, &C4H(dddd00ff), "Fill    %d", w->undoFill);
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 80), "Courier New", 12, &C4H(dddd00ff), "#Undo   %d", w->undoUndoLen);
	GUI_Printf_(gm, V(tl.x - 130, tl.y + 96), "Courier New", 12, &C4H(dddd00ff), "#Redo   %d", w->undoRedoLen);
//	GUI_Integer(w->undoCurrent, V(tl.x - 100, tl.y + 16), "Arial", 12, &C4H(ffffffff));
//	GUI_Integer(w->undoOldest, V(tl.x - 100, tl.y + 32), "Arial", 12, &C4H(ffffffff));
	
	// current undo head position
	GUI_Triangle(V(tl.x - 20, tl.y + w->undoCurrent * 16 + 8), 6, .25, 3.1416 / -2, &C4H(00ff00ff));
	
	// current undo head position
	GUI_Triangle(V(tl.x - 30, tl.y + w->undoNewest * 16 + 8), 6, .25, 3.1416 / -2, &C4H(ff0000ff));
	
	// ring divider
	GUI_Rect(V(tl.x, tl.y /*+ w->undoOldest * 16*/), V(sz.x, 1), &C4H(7777ffff));
	
	
	gm->curZ -= 1000000;
}


void BufferUndo_DebugRenderItem(GUIManager* gm, BufferUndo* u, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	char buf[256];

	GUI_TextLine(undo_actions_names[u->action], 0, tl, "Arial", 12, &C4H(ffffffff));
	
	switch(u->action) {
		case UndoAction_InsertChar:
			buf[0] = u->character;
			GUI_TextLine(buf, 1, V(tl.x + 40, tl.y), "Arial", 12, &C4H(88ff88ff));
			break;
			
		case UndoAction_InsertText:
			sprintf(buf, "%ld:%ld", u->lineNum, u->colNum);
			GUI_TextLine(buf, 0, V(tl.x + 70, tl.y), "Arial", 12, &C4H(88ff88ff));
			
			GUI_TextLine(u->text, u->length, V(tl.x + 100, tl.y), "Arial", 12, &C4H(ff8888ff));
			break;

		case UndoAction_DeleteText:
			sprintf(buf, "%ld:%ld", u->lineNum, u->colNum);
			GUI_TextLine(buf, 0, V(tl.x + 70, tl.y), "Arial", 12, &C4H(88ff88ff));
			
			GUI_TextLine(u->text, u->length, V(tl.x + 100, tl.y), "Arial", 12, &C4H(ff8888ff));
			break;

	}
}


#include "ui/macros_off.h"













