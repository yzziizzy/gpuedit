

#include "optionsEditor.h"
#include "ui/gui_internal.h"



// TODO: button or mode to highlight ascii character classes


#include "ui/macros_on.h"



static void text_field(OptionsEditor* oe, GUIManager* gm, vec2 tl, vec2 sz, PassFrameParams* pfp) {
	
}



static void save_edits(OptionsEditor* oe, int index) {
	if(oe->editingIndex == -1 || oe->editingIndex == index) return;
	
	if(oe->stringTarget) {
		if(*oe->stringTarget) free(*oe->stringTarget);
		*oe->stringTarget = strndup(oe->editString.data, oe->editString.len);
		
		oe->stringTarget = NULL;
	}
	
	if(oe->longTarget) {
		*oe->longTarget = oe->editLong;
		oe->longTarget = NULL;
	}
	if(oe->intTarget) {
		*oe->intTarget = oe->editLong;
		oe->intTarget = NULL;
	}
	
	if(oe->floatTarget) {
		*oe->floatTarget = oe->editFloat;
		oe->floatTarget = NULL;
	}
	
	if(oe->doubleTarget) {
		*oe->doubleTarget = oe->editDouble;
		oe->doubleTarget = NULL;
	}
	
	if(oe->boolTarget) {
		*oe->boolTarget = !!oe->editBool;
		oe->boolTarget = NULL;
	}

}



typedef struct {
	float yoff;
	int index;
	vec2 ntl, nsz; // the name
	vec2 vtl, vsz; // the value
	vec2 ltl, lsz; // the line
	
	vec2 tl, sz; // parent info
	
	f32 nameWidth;
	f32 valueWidth;
	f32 topMargin;
	f32 nameLeftMargin;
	f32 lineHeight;
	
	Color4 lineBGColor;
	Color4 trueColor;
	Color4 falseColor;
	
	OptionsEditor* w; // cached
} lineinfo_t;


static void set_line_params(lineinfo_t* b) {
	
	// the name
	b->ntl = V2(b->tl.x + b->nameLeftMargin, b->tl.y + b->yoff);
	b->nsz = V2(b->nameWidth, b->lineHeight);
	
	// the value
	b->vtl = V2(b->tl.x + b->nameLeftMargin + b->nameWidth, b->tl.y + b->yoff);
	b->vsz = V2(b->sz.x - b->nameLeftMargin - b->nameWidth, b->lineHeight);

	// the whole line
	b->ltl = V2(b->tl.x, b->tl.y + b->yoff);
	b->lsz = V2(b->sz.x, b->lineHeight);
}


static void next_line(lineinfo_t* b) {
	b->yoff += b->lineHeight;
	b->index++;
	
	set_line_params(b);
}
	

static void line_hover(lineinfo_t* b, GUIManager* gm, void* id) {
	if(b->w->editingIndex != b->index) {
		HOVER_HOT(id, b->ltl, b->lsz) 
		 
		if(CLICK_HOT_TO_ACTIVE(id)) { 
			save_edits(b->w, b->index); 
			b->w->editingIndex = b->index; 
		} 
		
		if(IS_HOT(id) && gm->drawMode) { 
			Z(-1); GUI_Rect(b->ltl, b->lsz, &b->lineBGColor); Z(1);
		} 
	} 
	else { 
		GUI_Box(b->ltl, b->lsz, 1, &C4H(dddd66ff)); 
	}
}


static void string_edit(lineinfo_t* b, GUIManager* gm, void* id, char** target) {
	if(b->w->editingIndex == b->index) {
		if(b->w->stringTarget != target) {
			b->w->stringTarget = target;
			GUIString_Set(&b->w->editString, *target ? *target : "");
		}
		
		GUI_Edit(&b->w->editString, b->vtl, b->vsz.x, &b->w->editString);
	}
	else {
		GUI_TextLineVCentered(b->vtl, b->vsz, *target, -1);
	}
}

static void int_edit(lineinfo_t* b, GUIManager* gm, void* id, int* target, long min, long max) {
	if(b->w->editingIndex == b->index) {
		if(b->w->intTarget != target) {
			b->w->intTarget = target;
			b->w->editLong = *target;
		}
		
		if(GUI_IntEdit(&b->w->editLong, b->vtl, b->vsz.x, &b->w->editLong)) {
			b->w->editLong = MIN(max, MAX(min, b->w->editLong));
		}
	}
	else {
		GUI_PrintfVCentered(b->vtl, b->vsz, "%d", *target);
	}
}

static void float_edit(lineinfo_t* b, GUIManager* gm, void* id, float* target, float min, float max) {
	if(b->w->editingIndex == b->index) {
		if(b->w->floatTarget != target) {
			b->w->floatTarget = target;
			b->w->editFloat = *target;
		}
		
		if(GUI_FloatEdit(&b->w->editFloat, b->vtl, b->vsz.x, &b->w->editFloat)) {
			b->w->editFloat = MIN(max, MAX(min, b->w->editFloat));
		}
	}
	else {
		GUI_PrintfVCentered(b->vtl, b->vsz, "%f", *target);
	}
}

static void bool_edit(lineinfo_t* b, GUIManager* gm, bool* target) {
	
	HOVER_HOT(target, b->ltl, b->lsz)
	
	if(CLICK_HOT_TO_ACTIVE(target)) { 
		save_edits(b->w, b->index); 
		b->w->editingIndex = b->index; 
		*target = !*target; 
	} 
	
	if(IS_HOT(target) && gm->drawMode) { 
		Z(-1); GUI_Rect(b->ltl, b->lsz, &b->lineBGColor); Z(1);
	} 
	
	GUI_TextLineAdv( 
		b->vtl, b->vsz, *target ? "true" : "false", -1, 
		GUI_TEXT_ALIGN_VCENTER, 
		gm->curFont, gm->curFontSize, 
		*target ? &b->trueColor : &b->falseColor 
	); 
}



void OptionsEditor_Render(OptionsEditor* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
	GUI_PushFontName("Arial", 14, &C4H(ddddddff));
	
	lineinfo_t b = {
		.nameWidth = 300,
		.topMargin = 10,
		.nameLeftMargin = 10,
		.lineHeight = 25,
		.index = 0,
		.tl = tl,
		.sz = sz,
		.w = w,
		.lineBGColor = C4H(777777ff),
		.trueColor = C4H(44ee66ff),
		.falseColor = C4H(ee6644ff),
	};
	b.valueWidth = sz.x - b.nameWidth - 20; // 20 for the scrollbar
	b.yoff = b.topMargin;
	b.yoff -= w->scrollPos;
	
	set_line_params(&b);
	
	// TODO: propagate changes through application
	// TODO: color selector
	// TODO: highlight changes
	GUI_PushClip(tl, sz);
	Z(2);


	#define BOOL_LINE_ITEM(name, target) \
		GUI_TextLineVCentered(b.ntl, b.nsz, name, -1); \
		bool_edit(&b, gm, &target); \
		next_line(&b);
	
	
	#define STRING_LINE_ITEM(name, target) \
		GUI_TextLineVCentered(b.ntl, b.nsz, name, -1); \
		string_edit(&b, gm, name, &target); \
		line_hover(&b, gm, &target); \
		next_line(&b);
	
	#define INT_LINE_ITEM(name, target, min, max) \
		GUI_TextLineVCentered(b.ntl, b.nsz, name, -1); \
		int_edit(&b, gm, name, &target, min, max); \
		line_hover(&b, gm, &target); \
		next_line(&b);
		
	
	#define FLOAT_LINE_ITEM(name, target, min, max) \
		GUI_TextLineVCentered(b.ntl, b.nsz, name, -1); \
		float_edit(&b, gm, name, &target, min, max); \
		line_hover(&b, gm, &target); \
		next_line(&b);
		
	

	
	#define SEPARATOR_LINE_ITEM(name, ...) {\
		f32 osz = gm->curFontSize; \
		gm->curFontSize *= 2.; \
		GUI_TextLineVCentered(b.ntl, V2(b.nsz.x, b.nsz.y * 2), name, -1); \
		gm->curFontSize = osz; \
		next_line(&b); \
		next_line(&b); \
	};
	

	
		

#define OPTION_LIST \
	X(SEPARATOR, "General Settings") \
	X(STRING, "Highlighters Path", w->gs->highlightersPath) \
	X(STRING, "Highlighter Styles Path", w->gs->highlightStylesPath) \
	X(STRING, "Commands Path", w->gs->commandsPath) \
	X(STRING, "Images Path", w->gs->imagesPath) \
	X(STRING, "GCC Base Path", w->gs->gccBasePath) \
	X(STRING, "GCC Error JSON Path", w->gs->gccErrorJSONPath) \
	X(STRING, "Theme Path", w->gs->Theme_path) \
	X(BOOL, "Enable Window Alpha", w->gs->windowAlpha) \
	X(BOOL, "Enable Sessions", w->gs->enableSessions) \
	X(BOOL, "Enable VSync", w->gs->enableVSync) \
	X(BOOL, "Open In Place", w->gs->MainControl_openInPlace) \
	X(BOOL, "Autosort Tabs", w->gs->MainControl_autoSortTabs) \
	X(BOOL, "Scroll Tab Names", w->gs->MainControl_scrollTabNames) \
	X(INT,  "Pane Tab Limit", w->gs->MainControl_paneTabLimit, 0, 1000) \
	X(INT,  "Framerate Limit", w->gs->frameRate, 0, 360) \
	X(SEPARATOR, "Buffer Settings") \
	X(INT,    "Lines Per Scroll Wheel", w->bs->linesPerScrollWheel, 1, 100) \
	X(BOOL,   "Disable Scrollbar", w->bs->disableScrollbar) \
	X(FLOAT,  "Scrollbar Fade Distance", w->bs->scrollbarFadeDistance, 1, 16384) \
	X(BOOL,   "Enable Cursor Blink", w->bs->cursorBlinkEnable) \
	X(FLOAT,  "Cursor Blink Off Time", w->bs->cursorBlinkOffTime, 0, 300000) \
	X(FLOAT,  "Cursor Blink On Time", w->bs->cursorBlinkOnTime, 0, 300000) \
	X(BOOL,   "Highlight Current Line", w->bs->highlightCurrentLine) \
	X(BOOL,   "Outline Current Line", w->bs->outlineCurrentLine) \
	X(FLOAT,  "Outline Current Line Y Offset", w->bs->outlineCurrentLineYOffset, -999, 999) \
	X(FLOAT,  "Line Num Extra Width", w->bs->lineNumExtraWidth, 0, 1920*16) \
	X(BOOL,   "Show Line Nums", w->bs->showLineNums) \
	X(INT,    "Line Num Base", w->bs->lineNumBase, 2, 36) \
	X(STRING, "Line Num Charset", w->bs->lineNumCharset) \
	X(FLOAT,  "Character width", w->bs->charWidth, 1, 1920*16) \
	X(FLOAT,  "Line Height", w->bs->lineHeight, 1, 1920*16) \
	X(INT,    "Tab Width (in spaces)", w->bs->tabWidth, 0, INT_MAX) \
	/*X(STRING, "Font Name", w->bs->font)*/ \
	X(FLOAT,  "Font Size", w->bs->fontSize, 1, 1920*16) \
	X(BOOL,   "Invert Selection", w->bs->invertSelection) \
	X(INT,    "Max Undo Steps", w->bs->maxUndo, 0, INT_MAX) \
	X(BOOL,   "Use Dictionary", w->bs->useDict) \
	X(STRING, "Dictionary Char Set", w->bs->dictCharSet) \
	X(INT,    "Status Bar Height", w->bs->statusBarHeight, 0, INT_MAX) \
	X(INT,    "Autocomplete Max Skip", w->bs->autocompleteMaxSkip, 0, 100) \
	/*X(STRING, "", w->bs->gccBasePath)*/ \
	/**X(STRING, "", w->bs->gccErrorJSONPath)*/ \
	/*X(STRING, "", w->bs->gccErrorJSONSuffix)*/ \

	
	
	
#define X(t, n, ...) CAT(t, _LINE_ITEM)(n, __VA_ARGS__)
	OPTION_LIST
#undef X

	
	// calculate the total height
	
	f32 STRING_height = b.lineHeight;
	f32 BOOL_height = b.lineHeight;
	f32 INT_height = b.lineHeight;
	f32 FLOAT_height = b.lineHeight;
	f32 SEPARATOR_height = b.lineHeight * 2.f;
#define X(t, ...) CAT(t, _height) +
	f32 totalHeight = b.topMargin + OPTION_LIST - sz.y;
#undef X



	
	if(GUI_InputAvailable()) {
		if(GUI_MouseWentDown(1) && GUI_MouseInside(tl, sz)) {
			save_edits(w, -1);
			w->editingIndex = -1;
			ACTIVE(NULL);
		}
	}
	
	
	Z(-2)
	GUI_PopFont();
	
	GUI_VScrollbar(&w->scrollPos, V2(tl.x + sz.x - 12, tl.y), V2(12, sz.y), totalHeight, &w->scrollPos);
	GUI_ScrollwheelRegion(tl, sz, totalHeight, b.lineHeight * 3, &w->scrollPos);
	
	// command processing
	if(!gm->drawMode && GUI_InputAvailable()) {
		size_t numCmds;
		GUI_Cmd* cmd = Commands_ProbeCommand(gm, GUIELEMENT_OptionsEditor, &gm->curEvent, &w->inputState, &numCmds);
		int needRehighlight = 0;
		for(int j = 0; j < numCmds; j++) { 
			if(!OptionsEditor_ProcessCommand(w, cmd+j)) {
				GUI_CancelInput();
			}
		}
		
		Commands_UpdateModes(gm, &w->inputState, cmd, numCmds);
	}
	
	GUI_PopClip();
}


#include "ui/macros_off.h"


int OptionsEditor_ProcessCommand(OptionsEditor* w, GUI_Cmd* cmd) {
//	GUIManager* gm = w->gm;
	
	switch(cmd->cmd){
//		case GUICMD_OptionsEditor_MoveCursorV:
//			w->cursor.pos += w->bytesPerLine * cmd->amt;
//			w->cursor.pos = MAX(0, MIN(w->cursor.pos, w->len - 1));
//			Hexedit_ScrollToCursor(w);
			break;
			
//		case GUICMD_OptionsEditor_MoveCursorH:
//			w->cursor.pos += cmd->amt;
//			w->cursor.pos = MAX(0, MIN(w->cursor.pos, w->len - 1));
//			Hexedit_ScrollToCursor(w);
			break;
		
//		case GUICMD_OptionsEditor_ScrollV:
//			w->scrollPos += cmd->amt;
//			w->scrollPos = MAX(0, MIN(w->scrollPos, 1 + w->len / w->bytesPerLine));
			break;
			
	
		default:
			return 1;
	}
	
	return 0;
}



void OptionsEditor_Init(OptionsEditor* oe, /*GUIManager* gm,*/ Settings* s, MessagePipe* tx) {
	
	oe->s = s;
	oe->gs = Settings_GetSection(s, SETTINGS_General);
	oe->bs = Settings_GetSection(s, SETTINGS_Buffer);

	oe->editingIndex = -1;
}

void OptionsEditor_Destroy(OptionsEditor* oe) {
	
	
}






