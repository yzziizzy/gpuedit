
#include <time.h>

#include "statusBar.h"
#include "ui/gui_internal.h"
#include "buffer.h"




static int sort_items_fn(void* a_, void* b_) {
	StatusBarItem* a = *((StatusBarItem**)a_);
	StatusBarItem* b = *((StatusBarItem**)b_);

	return a->order - b->order;
}

static void setLine(StatusBar* w, StatusBarItem* item);
static size_t strflinecol(char* s, size_t max, const char* format, GUIBufferEditControl* ec);


#include "ui/macros_on.h" 

void StatusBar_Render(StatusBar* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {

	if(!gm->drawMode) return;
	
	GUIFont* font = gm->defaults.font_fw;
	float fontSize = gm->gs->fontSize_fw;
	float charWidth = gm->gs->charWidth_fw;
	float lineHeight = gm->gs->lineHeight_fw;
	struct Color4 bg = gm->defaults.statusBarBgColor;
	struct Color4 text = gm->defaults.statusBarTextColor;
	
	
	// update sizes
	VEC_SORT(&w->left, sort_items_fn);
	VEC_SORT(&w->center, sort_items_fn);
	VEC_SORT(&w->right, sort_items_fn);
	
	float off = 0;
	VEC_EACH(&w->left, i, item) {
		item->offset = off;
		off += item->size * charWidth;
		setLine(w, item);
	}	
	
	off = sz.x;
	VEC_EACH(&w->right, i, item) {
		off -= item->size * charWidth;
		item->offset = off;
		setLine(w, item);
	}
	
	
	
	GUI_Rect(tl, sz, &gm->defaults.statusBarBgColor);
	
	gm->curZ++;
	// draw widgets
//	Vector2 tl = w->header.absTopLeft;
//	tl.y -= 500;
	Vector2 size = {0, lineHeight };
	Vector2 offset = tl;
	
	VEC_EACH(&w->items, i, item) {
		offset = tl;
		size.x = item->size * charWidth;
		offset.x = item->offset;
//		printf("widget '%s', size: %ld [%.2f,%.2f] at (%.2f, %.2f)\n", item->line, item->size, size.x, size.y, offset.x, offset.y);
		
		GUI_TextLine(
			item->line, strlen(item->line),
			offset,
			gm->defaults.fontName_fw, fontSize,
			&gm->defaults.statusBarTextColor
		);
	}	
	
	gm->curZ--;
}


#include "ui/macros_off.h" 


static size_t strflinecol(char* s, size_t max, const char* format, GUIBufferEditControl* ec) {
	size_t copied = 0;
	char buffer[20];
	int len = 0;
	
	for(int i = 0; format[i] != '\0'; i++) {
		switch(format[i]) {
			case '%':
				switch(format[i + 1]) {
					case 'L':
						len = snprintf(buffer, 20, "%ld", (int64_t)CURSOR_LINE(ec->sel)->lineNum);
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case 'C':
						len = snprintf(buffer, 20, "%ld", (int64_t)CURSOR_COL(ec->sel));
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case 'T':
						len = snprintf(buffer, 20, "%ld", (int64_t)ec->b->numLines);
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case '%':
						s[copied++] = '%';
						i++;
				}
				break;
				
			default:
				s[copied++] = format[i];
		}
	}
	
	s[copied++] = '\0';
	
	return copied;
}


static size_t strfbufmode(char* s, size_t max, const char* format, GUIBufferEditor* ed) {
	size_t copied = 0;
	char buffer[20];
	int len = 0;
	
	for(int i = 0; format[i] != '\0'; i++) {
		switch(format[i]) {
			case '%':
				switch(format[i + 1]) {
					case 'D':
						len = snprintf(buffer, 20, "%d", ed->ec->inputState.mode);
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case 'S':
						GUI_CmdModeInfo* info = ed->ec->inputState.modeInfo;
						if(info) {
							len = snprintf(buffer, 20, "%s", info->name);
							for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
							copied += len;
							i++;
						} else {
							s[copied++] = '-';
							i++;
						}
						break;
						
					case '%':
						s[copied++] = '%';
						i++;
				}
				break;
				
			default:
				s[copied++] = format[i];
		}
	}
	
	s[copied++] = '\0';
	
	return copied;
}



static size_t strffindstate(char* s, size_t max, const char* format, GUIBufferEditor* ed) {
	size_t copied = 0;
	char buffer[20];
	int len = 0;
	
	BufferFindState* st = ed->findState;
	if(!st) {
		char* msg = "no find state";
		len = strlen(msg);
		for(int j = 0; j < len; j++) memcpy(&s[copied], msg, len);
		copied += len;
		s[copied++] = '\0';
		return copied;
	}
	
	if(!st->findSet) {
		char* msg = "no find set";
		len = strlen(msg);
		for(int j = 0; j < len; j++) memcpy(&s[copied], msg, len);
		copied += len;
		s[copied++] = '\0';
		return copied;
	}
	
	int range_len = VEC_LEN(&st->findSet->ranges);
	if(!range_len) {
		char* msg = "no ranges";
		len = strlen(msg);
		for(int j = 0; j < len; j++) memcpy(&s[copied], msg, len);
		copied += len;
		s[copied++] = '\0';
		return copied;
	}
	
	
	for(int i = 0; format[i] != '\0'; i++) {
		switch(format[i]) {
			case '%':
				switch(format[i + 1]) {
					
					// I: current match index
					// N: total matches
					
					case 'I':
						len = snprintf(buffer, 20, "%ld", st->findIndex + 1);
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case 'N':
						len = snprintf(buffer, 20, "%d", range_len);
						for(int j = 0; j < len; j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
						
					case '%':
						s[copied++] = '%';
						i++;
				}
				break;
				
			default:
				s[copied++] = format[i];
		}
	}
	
	s[copied++] = '\0';
	
	return copied;
}



static void setLine(StatusBar* w, StatusBarItem* item) {
	switch(item->type) {
		case MCWID_HELLO:
			strcpy(item->line, "hello world");
			break;
			
		case MCWID_PING:
			strcpy(item->line, "ping stats");
			break;
			
		case MCWID_CLOCK: {
			time_t timer = time(NULL);
			struct tm* tm = localtime(&timer);
			strftime(item->line, 100, item->format, tm);
			break;
		}
		
		case MCWID_BATTERY:
			strcpy(item->line, "batt: over 9000%");
			break;
		
		case MCWID_BUFMODE:
			strfbufmode(item->line, 100, item->format, w->ed);
			break;
		
		case MCWID_LINECOL:
			strflinecol(item->line, 100, item->format, w->ec);
			break;
		
		case MCWID_FINDSTATE:
			strffindstate(item->line, 100, item->format, w->ed);
			break;
		
		case MCWID_NONE:
		default:
			break;
	}
}



StatusBar* StatusBar_New(GUIManager* gm, GUIBufferEditor* ed) {
	
	StatusBar* w = pcalloc(w);
	w->ed = ed;
	w->ec = ed->ec;

	return w;
}


StatusBarItem* StatusBar_AddItem(StatusBar* w, WidgetSpec* spec, int order) {
	
	StatusBarItem* it = pcalloc(it);
	
	it->type = spec->type;
	it->size = spec->size;
	it->align = spec->align;
	it->order = order;
	it->format = spec->format;
	
	it->offset = 0;
	
	VEC_PUSH(&w->items, it);
	
	if(it->align == 'l') {
		VEC_PUSH(&w->left, it);
		VEC_SORT(&w->left, sort_items_fn);
	}
	else if(it->align == 'c') {
		VEC_PUSH(&w->center, it);
		VEC_SORT(&w->center, sort_items_fn);
	}
	else if(it->align == 'r') {
		VEC_PUSH(&w->right, it);
		VEC_SORT(&w->right, sort_items_fn);
	}
	
	return it;
}


StatusBar* StatusBar_SetItems(StatusBar* w, WidgetSpec* widgets) {
	for(int i = 0; widgets[i].type; i++) {
		StatusBar_AddItem(w, &widgets[i], i);
	}
	
	return w;
}





