
#include <ctype.h>

#include "calcControl.h" 
#include "ui/gui_internal.h"

#include "sti/rpn.h"




static size_t parse(char* input, char*** output);
static void setAnswer(GUICalculatorControl* w, double d);



static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	GUIHeader* h = &w->header;
	GUIManager* gm = h->gm;
	
	Vector2 tl = h->absTopLeft;	
	
	gui_drawTextLine(gm, 
		(Vector2){tl.x + 20, tl.y + h->size.y - 50}, 
		(Vector2){h->size.x - 20, 20}, 
		&h->absClip, 
		&gm->defaults.selectedItemTextColor, h->absZ + 0.1, 
		w->answer, strlen(w->answer)
	);
	

	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;

	w->history->header.size.y = w_->size.y - (w->inputBox->header.size.y * 2);

	gui_defaultUpdatePos(&w->header, grp, pfp);
}






static void reap(GUIHeader* w_) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	if(w->answer) {
		free(w->answer);
		w->answer = NULL;
	}
}


static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	
	GUIManager_pushFocusedObject(w->header.gm, &w->inputBox->header);
}



static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	GUICalculatorControl_ProcessCommand(w, cmd);
}

void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
		case FuzzyMatcherCmd_Exit:
			GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");			
			break;
			
		case FuzzyMatcherCmd_CursorMove:
//			if(w->matchCnt == 0) break;
//			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;
			
	}
	
}




static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	
	if((GUIEdit*)gev->originalTarget == w->inputBox) {
		if(0 == strcmp(gev->userType, "change")) {
						
//			if(w->searchTerm) free(w->searchTerm);
//			w->searchTerm = strndup(gev->userData, gev->userSize);
			
			GUICalculatorControl_Refresh(w);
		}
	}
}


GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Reap = reap,
		.UpdatePos = (void*)updatePos,
		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
//		.KeyDown = keyDown,
		.GainedFocus = gainedFocus,
		//.Click = click,
		//.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
		.User = userEvent,
	};
	
	
	GUICalculatorControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	w->header.cmdElementType = CUSTOM_ELEM_TYPE_Calc;
	
	
	w->inputBox = GUIEdit_New(gm, "");
	
	w->inputBox->header.flags |= GUI_MAXIMIZE_X;
	w->inputBox->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->inputBox->header.topleft.y = 0;
	w->inputBox->header.topleft.x = 0;
	w->inputBox->header.size.y = 25;
	
	
	w->history = GUIStringList_New(gm);
	w->history->header.flags |= GUI_MAXIMIZE_X;
	w->history->header.size.y = 200;
	w->history->maxItems = 20;
	
	w->ansalloc = 128;
	w->answer = calloc(1, sizeof(*w->answer) * w->ansalloc);
	
	
	GUI_RegisterObject(w, w->inputBox);
	GUI_RegisterObject(w, w->history);
	
	return w;
}



static void setAnswer(GUICalculatorControl* w, double d) {
	snprintf(w->answer, w->ansalloc, "%f", d);
	GUIStringList_PrependItem(w->history, w->answer);
}





char charcats[] = {
	[0] = 0, [1] = 0, [2] = 0, [3] = 0, [4] = 0,
	[5] = 0, [6] = 0, [7] = 0, [8] = 0, 
	['\t'] = 1, ['\n'] = 1, ['\v'] = 1, ['\f'] = 1, ['\r'] = 1, // whitespace
	[0x0e] = 0,
	[0x0f] = 0,
	[0x10] = 0, [0x11] = 0, [0x12] = 0, [0x13] = 0, [0x14] = 0, [0x15] = 0,
	[0x16] = 0, [0x17] = 0, [0x18] = 0, [0x19] = 0, 
	[' '] = 1,
	['!'] = 2, // symbol
	['"'] = 2, 
	['#'] = 2, 
	['$'] = 2, 
	['%'] = 2, 
	['&'] = 2, 
	['\''] = 2, 
	['('] = -1, // paren
	[')'] = -1, 
	['*'] = 2, 
	['+'] = 2, 
	[','] = 2, 
	['-'] = 2, 
	['.'] = 3, // number
	['/'] = 2, 
	['0'] = 3, ['1'] = 3, ['2'] = 3, ['3'] = 3, ['4'] = 3,
	['5'] = 3, ['6'] = 3, ['7'] = 3, ['8'] = 3, ['9'] = 3,
	[':'] = 2,
	[';'] = 2,
	['<'] = 2,
	['='] = 2,
	['>'] = 2,
	['?'] = 2,
	['@'] = 2,
	['A'] = 4, ['B'] = 4, ['C'] = 4, ['D'] = 4, ['E'] = 4, ['F'] = 4, ['G'] = 4, 
	['H'] = 4, ['I'] = 4, ['J'] = 4, ['K'] = 4, ['L'] = 4, ['M'] = 4, ['N'] = 4, 
	['O'] = 4, ['P'] = 4, ['Q'] = 4, ['R'] = 4, ['S'] = 4, ['T'] = 4, ['U'] = 4, 
	['V'] = 4, ['W'] = 4, ['X'] = 4, ['Y'] = 4, ['Z'] = 4,
	['['] = -1,
	['\\'] = 2,
	[']'] = -1,
	['^'] = 2,
	['_'] = 2,
	['`'] = 2,
	['a'] = 4, ['b'] = 4, ['c'] = 4, ['d'] = 4, ['e'] = 4, ['f'] = 4, ['g'] = 4, 
	['h'] = 4, ['i'] = 4, ['j'] = 4, ['k'] = 4, ['l'] = 4, ['m'] = 4, ['n'] = 4, 
	['o'] = 4, ['p'] = 4, ['q'] = 4, ['r'] = 4, ['s'] = 4, ['t'] = 4, ['u'] = 4, 
	['v'] = 4, ['w'] = 4, ['x'] = 4, ['y'] = 4, ['z'] = 4, 
	['{'] = -1, 
	['|'] = 2, 
	['}'] = -1, 
	['~'] = 2, 
	[0x7f] = 1, 


};




static size_t parse(char* input, char*** output) {
	char* s = input;
	size_t alloc = 32;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	
	for(; *s; ) {
		if(len >= alloc) {
			alloc *= 2;
			out = realloc(out, alloc * sizeof(*out));
		}
		
		if(isalnum(*s) || *s == '.') {
			char* e = s;
			while(*e && (isalnum(*e) || *e == '.')) e++;
			
			out[len] = strndup(s, e - s);
			len++;
			
			s = e;
			continue;
		}
		
		switch(*s) {
			case '*':
			case '/':
			case '+':
			case '-':
			case '(':
			case ')':
			case '[':
			case ']':
			case ',':
			case ';':
				out[len] = strndup(s, 1);
				len++;
				s++;
				break;
				
			case ' ':
			case '\r':
			case '\n':
			default:
				s++;
				continue;
		}
	}
	
	// null-terminate the list
	if(len >= alloc) {
		out = realloc(out, (alloc+1) * sizeof(*out));
	}
	out[len] = NULL;
		
	*output = out;
	
	return len;
}





static void freeptrlist(void* _p) {
	void** p = (void**)_p;
	void** q = p;
	while(*q) {
		free(*q);
		q++;
	}
	free(p);
}

void GUICalculatorControl_Refresh(GUICalculatorControl* w) {
	
	char* q = GUIEdit_GetText(w->inputBox);
	
	
	char** tokens;
	size_t qlen = parse(q, &tokens);
	
	

	sti_op_prec_rule rules[] = {
		{"",   0, STI_OP_ASSOC_NONE,  0},
		{"+",  1, STI_OP_ASSOC_LEFT,  2},
		{"-",  1, STI_OP_ASSOC_LEFT,  2},
		{"*",  2, STI_OP_ASSOC_LEFT,  2},
//		{"**", 3, STI_OP_ASSOC_LEFT,  2},
		{"/",  2, STI_OP_ASSOC_LEFT,  2},
		{"(",  8, STI_OP_OPEN_PAREN,  0},
		{")",  8, STI_OP_CLOSE_PAREN, 0},
		{"[",  9, STI_OP_OPEN_PAREN,  0},
		{"]",  9, STI_OP_CLOSE_PAREN, 0},
		{NULL, 0, 0, 0},
	};



	char** rpn;
	size_t rlen = 0;
	int ret = 0;
	
	ret = infix_to_rpn(rules, tokens, &rpn, &rlen);
	if(!ret) {
		setAnswer(w, rpn_eval_double_str(rpn));
		free(rpn);
	}
	
	// rpn is a list of the same strings from tokens
	freeptrlist(tokens);
}


