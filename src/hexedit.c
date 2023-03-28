
#include <ctype.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#include "hexedit.h"
#include "ui/gui_internal.h"






#include "ui/macros_on.h"


void Hexedit_Render(Hexedit* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
	w->linesOnScreen = sz.y / w->bs->lineHeight;
	w->bytesPerLine = 16;
	
	// command processing
	if(!gm->drawMode && GUI_InputAvailable()) {
		size_t numCmds;
		GUI_Cmd* cmd = Commands_ProbeCommand(gm, GUIELEMENT_Hexedit, &gm->curEvent, &w->inputState, &numCmds);
		int needRehighlight = 0;
		for(int j = 0; j < numCmds; j++) { 
			if(!Hexedit_ProcessCommand(w, cmd+j)) {
				GUI_CancelInput();
			}
		}
		
		Commands_UpdateModes(gm, &w->inputState, cmd, numCmds);
	}
	
	
	//
	//  Drawing only bepos here
	//
	if(!gm->drawMode) return;
	
		
	gm->curZ++;
	
	uint8_t* data = w->data + w->bytesPerLine * w->scrollPos;
	long total = w->scrollPos * w->bytesPerLine;
	
	float nw = 8 * w->bs->charWidth;
	float off = w->bs->charWidth * (w->bytesPerLine * 3 + 4);
	
	for(int l = 0; l < w->linesOnScreen; l++) {
	
		int charCount;
		char txt[32];
		
		charCount = snprintf(txt, 32, "%.6lx", data - w->data);
				
		drawTextLine(gm, w->bs, w->font, &C4H(ffffffff), txt, charCount,  V(tl.x, tl.y + (l + 1) * w->bs->lineHeight));
	
	
		for(int b = 0; b < w->bytesPerLine; b++) {
			int c = data[b];
			
			int lc = c & 15; 
			int hc = (c >> 4) & 15; 
			
			int lc_char = lc < 10 ? '0' + lc : 'a' + (lc - 10);
			int hc_char = hc < 10 ? '0' + hc : 'a' + (hc - 10);
			
			GUI_CharFont_NoGuard(lc_char, V(tl.x + nw + (b*3+0) * w->bs->charWidth, tl.y + (l + 1) * w->bs->lineHeight), w->font, w->bs->fontSize, &C4H(ffffffff));
			GUI_CharFont_NoGuard(hc_char, V(tl.x + nw + (b*3+1) * w->bs->charWidth, tl.y + (l + 1) * w->bs->lineHeight), w->font, w->bs->fontSize, &C4H(ffffffff));
			
			
			int ac = c;
			if(!isprint(c)) ac = '.';
			
			GUI_CharFont_NoGuard(ac, V(tl.x + nw + off + b * w->bs->charWidth, tl.y + (l + 1) * w->bs->lineHeight), w->font, w->bs->fontSize, &C4H(ffffffff));
		
			if(++total >= w->len) goto DONE;
		} 
		
		data += w->bytesPerLine;
	}
DONE:

	gm->curZ += 10;
	
	int curline = w->cursor.pos / w->bytesPerLine - w->scrollPos;
	int curcol = w->cursor.pos % w->bytesPerLine;
	
	GUI_Box(
		V(tl.x - 1 + nw + (3*curcol*w->bs->charWidth), tl.y + 2 + (curline) * w->bs->lineHeight), 
		V(w->bs->charWidth * 2 + 2, w->bs->lineHeight + 2),
		1,
		&C4H(ff0000ff)
	);
	
	GUI_Box(
		V(tl.x - 1 + nw + off + curcol * w->bs->charWidth, tl.y + 2 + (curline) * w->bs->lineHeight), 
		V(w->bs->charWidth * 1 + 2, w->bs->lineHeight + 2),
		1,
		&C4H(ff0000ff)
	);
	gm->curZ -= 10;
	
	gm->curZ--;
}


#include "ui/macros_off.h"


void Hexedit_ScrollToCursor(Hexedit* w) {

	int cline = w->cursor.pos / w->bytesPerLine;
	
	if(cline < w->scrollPos) w->scrollPos = cline;
	if(cline > w->scrollPos + w->linesOnScreen - 1) w->scrollPos = cline - w->linesOnScreen + 1;
	
}


int Hexedit_ProcessCommand(Hexedit* w, GUI_Cmd* cmd) {
	GUIManager* gm = w->gm;
	
	switch(cmd->cmd){
		case GUICMD_Hexedit_MoveCursorV:
			w->cursor.pos += w->bytesPerLine * cmd->amt;
			w->cursor.pos = MAX(0, MIN(w->cursor.pos, w->len - 1));
			Hexedit_ScrollToCursor(w);
			break;
			
		case GUICMD_Hexedit_MoveCursorH:
			w->cursor.pos += cmd->amt;
			w->cursor.pos = MAX(0, MIN(w->cursor.pos, w->len - 1));
			Hexedit_ScrollToCursor(w);
			break;
		
		case GUICMD_Hexedit_ScrollV:
			w->scrollPos += cmd->amt;
			w->scrollPos = MAX(0, MIN(w->scrollPos, 1 + w->len / w->bytesPerLine));
			break;
			
	
		default:
			return 1;
	}
	
	return 0;
}



Hexedit* Hexedit_New(GUIManager* gm, Settings* s, char* path) {
	Hexedit* w = pcalloc(w);
	
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	w->bs = Settings_GetSection(s, SETTINGS_Buffer);
	
	w->font = GUI_FindFont(gm, w->bs->font, w->bs->fontSize);
	
	if(path) {
		Hexedit_LoadFile(w, path); 
	}
	
	return w;
}



void Hexedit_LoadFile(Hexedit* w, char* path) {
	struct stat sb;
	
	w->filePath = strdup(path);
	
	w->fd = open(path, 0);
	if(-1 == w->fd) {
		L1("Could not open file '%s'\n", path);
	}
	
	fstat(w->fd, &sb);
	
	w->len = sb.st_size;	
//	printf("file size: %ld\n", w->len);
	
	w->data = mmap(NULL, w->len, PROT_READ | PROT_WRITE, MAP_PRIVATE, w->fd, 0);
	if(MAP_FAILED == w->data) {
		L1("Could not map file '%s'\n", path);
	}
	
	w->bytesPerLine = 0;
	w->linesOnScreen = 0;
	w->scrollPos = 0;
}








