
#include <ctype.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


#include "hexedit.h"
#include "ui/gui_internal.h"






#include "ui/macros_on.h"


static void drawHexByte(Hexedit* w, GUIManager* gm, int c, Vector2 pos, Color4* color) {
	int lc = c & 15; 
	int hc = (c >> 4) & 15; 
	
	int lc_char = lc < 10 ? '0' + lc : 'a' + (lc - 10);
	int hc_char = hc < 10 ? '0' + hc : 'a' + (hc - 10);
	
	GUI_Char_NoGuard(hc_char, pos, w->font, w->bs->fontSize, color);
	GUI_Char_NoGuard(lc_char, V(pos.x + w->bs->charWidth, pos.y), w->font, w->bs->fontSize, color);
} 

static void drawHex(Hexedit* w, GUIManager* gm, void* d, int bytes, Vector2 pos, Color4* color) {
	uint64_t b;
	
	switch(bytes) {
		case 1: b = *(uint8_t*)d;
		case 2: b = *(uint16_t*)d;
		case 4: b = *(uint32_t*)d;
		case 8: b = *(uint64_t*)d;
	}

	
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		
		int a = b & 15;
		int a_char = a < 10 ? '0' + a : 'a' + (a - 10);
		
		GUI_Char_NoGuard(a_char, V(pos.x + i * w->bs->charWidth, pos.y), w->font, w->bs->fontSize, color);
		
		b >>= 4;
	}
	
} 

// line is relative to the start of the range
void drawRangeLine(Hexedit* w, GUIManager* gm, HexRange* r, ssize_t line, Vector2 tl) {
	
	uint8_t* data = w->data + r->pos + line * w->bytesPerLine;
	
	int bytesPerCol = 1;
	long total = line * w->bytesPerLine;

	float x = tl.x;
	for(int b = 0; b < w->bytesPerLine; b += bytesPerCol) {
		drawHex(w, gm, data + b, bytesPerCol, V(x + (b >= 8 ? w->bs->charWidth : 0), tl.y), &C4H(ffffffff));
		
		x += (bytesPerCol * 2 + 1) * w->bs->charWidth;
		
		if(++total >= r->len) break;
	}

}


void Hexedit_Render(Hexedit* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
	w->linesOnScreen = sz.y / w->bs->lineHeight;
	
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
	//  Drawing only below here
	//
	if(!gm->drawMode) return;
	
		
	gm->curZ++;
	
	int bytesPerCol = 1;
	
	uint8_t* data = w->data + w->bytesPerLine * w->scrollPos;
	long total = w->scrollPos * w->bytesPerLine;
	
	float nw = 8 * w->bs->charWidth;
	float off = w->bs->charWidth * (w->bytesPerLine * 3 + 4);
	
	
	// find the range for the first line to be displayed
	HexRange* r = Hexedit_FindRangeForLine(w, w->scrollPos);
	
	
	for(int l = 0; l < w->linesOnScreen; l++) {
	
		// byte index (line num)
		int charCount;
		char txt[32];
		
		charCount = snprintf(txt, 32, "%.6lx", data - w->data);
		drawTextLine(gm, w->bs, w->font, &C4H(ffffffff), txt, charCount,  V(tl.x, tl.y + (l + 1) * w->bs->lineHeight));
	
		// raw data
		drawRangeLine(w, gm, r, l + w->scrollPos, V(tl.x + nw, tl.y + (l + 1) * w->bs->lineHeight));
		
		// ascii side output
		for(int b = 0; b < w->bytesPerLine; b++) {
			int c = data[b];
			int ac = c;
			if(!isprint(c)) ac = '.';
			
			GUI_Char_NoGuard(ac, V(tl.x + nw + off + b * w->bs->charWidth, tl.y + (l + 1) * w->bs->lineHeight), w->font, w->bs->fontSize, &C4H(ffffffff));
		
			if(++total >= w->len) goto DONE;
		} 
		
		data += w->bytesPerLine;
	}
DONE:

	gm->curZ += 10;
	
	int curline = w->cursor.pos / w->bytesPerLine - w->scrollPos;
	int curcol = w->cursor.pos % w->bytesPerLine;
	
	GUI_Box(
		V(tl.x - 1 + nw + (3*curcol*w->bs->charWidth) + (curcol >= 8 ? w->bs->charWidth : 0), tl.y + 2 + (curline) * w->bs->lineHeight), 
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
	
	w->font = GUI_FindFont(gm, w->bs->font);
	
	if(path) {
		Hexedit_LoadFile(w, path); 
	}
	
	return w;
}

HexRange* HexRange_New(ssize_t start, ssize_t len, enum HexType type) {
	HexRange* r = pcalloc(r);
	r->pos = start;
	r->len = len;
	r->type = type;
	
	return r;
}

HexRange* Hexedit_FindRange(Hexedit* w, ssize_t pos) {
	HexRange* r = w->ranges;
	
	while(r) {
		if(pos >= r->pos && pos <= r->pos + r->len) return r;
	
		if(!r->next) return r;
		r = r->next;
	}
	
	return r;
}

HexRange* Hexedit_FindRangeForLine(Hexedit* w, ssize_t line) {
	HexRange* r = w->ranges;
	
	while(r) {
		if(r->len >= line) return r;
		line -= r->numLines;
		
		if(!r->next) return r;
		r = r->next;
	}
	
	return r;
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
	
	w->bytesPerLine = 16;
	w->linesOnScreen = 0;
	w->scrollPos = 0;
	
	w->ranges = HexRange_New(0, w->len, HEXEDIT_TYPE_u8);
	w->ranges->numLines = w->len / w->bytesPerLine + !!(w->len % w->bytesPerLine);
}








