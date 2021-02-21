
#include <time.h>

#include "statusBar.h"
#include "ui/gui_internal.h"




static int sort_items_fn(void* a_, void* b_) {
	GUIStatusBarItem* a = *((GUIStatusBarItem**)a_);
	GUIStatusBarItem* b = *((GUIStatusBarItem**)b_);

	return a->order - b->order;
}



static void render(GUIStatusBar* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	
	GUIHeader* h = &w->header;
	Vector2 tl = h->absTopLeft;
	
	GUIFont* font = FontManager_findFont(gm->fm, gm->gs->font_fw);
	float fontSize = gm->gs->fontSize_fw;
	float charWidth = gm->gs->charWidth_fw;
	float lineHeight = gm->gs->lineHeight_fw;
	struct Color4 bg = gm->defaults.statusBarBgColor;
	struct Color4 text = gm->defaults.statusBarTextColor;
	
	gui_drawBox(gm, h->absTopLeft, h->size, &h->absClip, h->absZ + 0.0001, &bg); 
	
	// draw widgets
//	Vector2 tl = w->header.absTopLeft;
//	tl.y -= 500;
	Vector2 size = { .x=0, .y=lineHeight };
	Vector2 offset = h->absTopLeft;
	VEC_EACH(&w->items, i, item) {
		size.x = item->size * charWidth;
		offset.x = item->offset;
//		printf("widget '%s', size: %ld [%.2f,%.2f] at (%.2f, %.2f)\n", item->line, item->size, size.x, size.y, offset.x, offset.y);
		gui_drawTextLineAdv(
			h->gm,
			offset,
			size,
			&h->absClip,
			&text,
			font,
			7.2 / fontSize, // magic
			9001,
			item->line,
			strlen(item->line)
		);
	}	
	
	GUIHeader_renderChildren(h, pfp);
}


static void delete(GUIStatusBar* w) {
	
}


static size_t strflinecol(char* s, size_t max, const char* format, GUIBufferEditControl* ec) {
	size_t copied = 0;
	char buffer[20];
	int len = 0;
	
	for(int i=0;format[i] != '\0';i++) {
		switch(format[i]) {
			case '%':
				switch(format[i+1]) {
					case 'L':
						len = snprintf(buffer, 20, "%ld", ec->current->lineNum);
						for(int j=0;j<len;j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
					case 'C':
						len = snprintf(buffer, 20, "%ld", ec->curCol);
						for(int j=0;j<len;j++) memcpy(&s[copied], buffer, len);
						copied += len;
						i++;
						break;
					case 'T':
						len = snprintf(buffer, 20, "%ld", ec->buffer->numLines);
						for(int j=0;j<len;j++) memcpy(&s[copied], buffer, len);
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


static void setLine(GUIStatusBar* w, GUIStatusBarItem* item) {
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
			strcpy(item->line, "batt: 100%");
			break;
		case MCWID_LINECOL:
			strflinecol(item->line, 100, item->format, w->ec);
			break;
		case MCWID_NONE:
		default:
			break;
	}
}


static void updatePos(GUIStatusBar* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	float charWidth = h->gm->gs->charWidth_fw;
	
	VEC_SORT(&w->left, sort_items_fn);
	VEC_SORT(&w->center, sort_items_fn);
	VEC_SORT(&w->right, sort_items_fn);
	
	float offset = 0;
	VEC_EACH(&w->left, i, item) {
		item->offset = offset;
		offset += item->size * charWidth;
		setLine(w, item);
	}	
	
	offset = h->size.x;
	VEC_EACH(&w->right, i, item) {
		offset -= item->size * charWidth;
		item->offset = offset;
		setLine(w, item);
	}
	
	gui_defaultUpdatePos(h, grp, pfp);
}


GUIStatusBar* GUIStatusBar_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
// 		.Delete = (void*)delete,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.Click = click,
	};
	
	GUIStatusBar* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	return w;
}


GUIStatusBarItem* GUIStatusBar_AddItem(GUIStatusBar* w, WidgetSpec* spec, int order) {
	
	GUIStatusBarItem* it = pcalloc(it);
	
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


GUIStatusBar* GUIStatusBar_SetItems(GUIStatusBar* w, WidgetSpec* widgets) {
	for(int i=0; widgets[i].type;i++) {
		GUIStatusBar_AddItem(w, &widgets[i], i);
	}
	
	return w;
}





