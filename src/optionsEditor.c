

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

void OptionsEditor_Render(OptionsEditor* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
//	w->linesOnScreen = sz.y / w->bs->lineHeight;
		
	
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
	
	GUI_PushFontName("Arial", 14, &C4H(ddddddff));
	
	Color4 lineBGColor = C4H(777777ff);
	Color4 trueColor = C4H(44ee66ff);
	Color4 falseColor = C4H(ee6644ff);
	f32 topMargin = 10;
	f32 nameLeftMargin = 10;
	float lh = 25;
	
	int index = 0;
	f32 yoff = topMargin;
	
	
	// TODO: highlight changes
	
	// the name
	#define ntl V2(tl.x + nameLeftMargin, tl.y + yoff)
	#define nsz V2(300, lh)
	
	// the value
	#define vtl V2(tl.x + nameLeftMargin + 300, tl.y + yoff)
	#define vsz V2(sz.x - nameLeftMargin - 300, lh)
	
	// the whole line
	#define ltl V2(tl.x, tl.y + yoff)
	#define lsz V2(sz.x, lh)
	
	#define NEXT_LINE do {\
		yoff += lh; \
		index++; \
	} while(0);
	
	
	Z(2);
	
	#define LINE_HOVER(id) \
		if(w->editingIndex != index) { \
			HOVER_HOT(id, ltl, lsz) \
			Z(-1) \
			if(CLICK_HOT_TO_ACTIVE(id)) { \
				save_edits(w, index); \
				w->editingIndex = index; \
			} \
			if(IS_HOT(id) && gm->drawMode) { \
				GUI_Rect(ltl, lsz, &lineBGColor); \
			} \
			Z(1); \
		} \
		else { \
			GUI_Box(ltl, lsz, 1, &C4H(dddd66ff)); \
		}
		
	
	#define STRING_EDIT(target) \
		if(w->editingIndex == index) { \
			if(w->stringTarget != &(target)) { \
				w->stringTarget = &(target); \
				GUIString_Set(&w->editString, (target)); \
			} \
			\
			GUI_Edit(&w->editString, vtl, vsz.x, &w->editString); \
		} \
		else { \
			GUI_TextLineVCentered(vtl, vsz, (target), -1); \
		}
		
		
	#define BOOL_EDIT(target) \
		HOVER_HOT(&(target), ltl, lsz) \
		if(CLICK_HOT_TO_ACTIVE(&(target))) { \
			save_edits(w, index); \
			w->editingIndex = index; \
			target = !target; \
		} \
		if(IS_HOT(&(target)) && gm->drawMode) { \
			Z(-1); GUI_Rect(ltl, lsz, &lineBGColor); Z(1) \
		} \
		GUI_TextLineAdv( \
			vtl, vsz, target ? "true" : "false", -1, \
			GUI_TEXT_ALIGN_VCENTER, \
			gm->curFont, \
			gm->curFontSize, \
			target ? &trueColor : &falseColor \
		); \
	
	
	// ----------------
	#define BOOL_LINE_ITEM(name, target) \
		GUI_TextLineVCentered(ntl, nsz, name, -1); \
		BOOL_EDIT(target) \
		NEXT_LINE
	
	#define STRING_LINE_ITEM(name, target) \
		GUI_TextLineVCentered(ntl, nsz, name, -1); \
		STRING_EDIT(target) \
		LINE_HOVER(&target) \
		NEXT_LINE
		
		
	
	STRING_LINE_ITEM("Highlighters Path", w->gs->highlightersPath)
	STRING_LINE_ITEM("Highlighter Styles Path", w->gs->highlightStylesPath)
	STRING_LINE_ITEM("Commands Path", w->gs->commandsPath)
	STRING_LINE_ITEM("Images Path", w->gs->imagesPath)
	STRING_LINE_ITEM("GCC Base Path", w->gs->gccBasePath)
	STRING_LINE_ITEM("GCC Error JSON Path", w->gs->gccErrorJSONPath)
	STRING_LINE_ITEM("Theme Path", w->gs->Theme_path)


	BOOL_LINE_ITEM("Enable Window Alpha", w->gs->windowAlpha)
	BOOL_LINE_ITEM("Enable Sessions", w->gs->enableSessions)
	BOOL_LINE_ITEM("Enable VSync", w->gs->enableVSync)
	BOOL_LINE_ITEM("Open In Place", w->gs->MainControl_openInPlace)
	BOOL_LINE_ITEM("Autosort Tabs", w->gs->MainControl_autoSortTabs)
	BOOL_LINE_ITEM("Scroll Tab Names", w->gs->MainControl_scrollTabNames)



//	GUI_Edit(w->gs->highlightersPath, tlc, 120, &w->highlightersPath);
	
	if(GUI_InputAvailable()) {
		if(GUI_MouseWentDown(1) && GUI_MouseInside(tl, sz)) {
			save_edits(w, -1);
			w->editingIndex = -1;
			ACTIVE(NULL);
		}
	}
	
	
	Z(-2)
	GUI_PopFont();
	
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
	oe->lastEditingIndex = -1;
}

void OptionsEditor_Destroy(OptionsEditor* oe) {
	
	
}






