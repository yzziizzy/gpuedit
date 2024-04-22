
#include <ctype.h>

#include "calcControl.h" 
#include "ui/gui_internal.h"

#include "sti/rpn.h"




static size_t parse(char* input, char*** output);
static void setAnswer(GUICalculatorControl* w, double d);



#include "ui/macros_on.h"

void GUICalculatorControl_Render(GUICalculatorControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
//	if(GUI_MouseInside(tl, sz)) {
//		ACTIVE(&w->searchTerm);
//	}

	if(gm->activeID == w) {
		ACTIVE(&w->searchTerm);
	}
	

	gm->curZ += 10;
	
	DEFAULTS(GUIEditOpts, eopts);
	eopts.selectAll = 1;
	if(GUI_Edit_(gm, &w->searchTerm, tl, sz.x, &w->searchTerm, &eopts)) {
		GUIFuzzyMatchControl_Refresh(w);
	}
	gm->curZ -= 10;


	if(GUI_InputAvailable()) {
		GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_FuzzyMatcher, &gm->curEvent, 0, NULL);
		
		if(cmd) {
			int cmd_result = GUIFuzzyMatchControl_ProcessCommand(w, cmd);
			switch(cmd_result) {
				case 0:
					GUI_CancelInput();
					break;
				case 1:
					// command not handled
					break;
				case 2: // editor is gone or other reason to process no more commands
					GUI_CancelInput();
					return;
				default:
					dbg("unexpected GUIFuzzyMatchControl_ProcessCommand result [%d]", cmd_result);
			}
		}
		
		if(GUI_MouseWentUp(1)) {
			if(GUI_MouseInside(tl, sz)) {
				ACTIVE(&w->searchTerm);
			}
			// determine the clicked line
			Vector2 mp = GUI_MousePos();
			int cline = floor((mp.y - 20) / w->lineHeight) - 1;
			
			if(cline >= 0 && cline <= (int)w->matchCnt - 1) {
				open_match(w, cline);
				GUI_CancelInput();
			}
		}
	}


	if(!gm->drawMode) return;


	// draw general background
	GUI_Rect(tl, sz, &gm->defaults.windowBgColor);
	
	// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;
	
	int linesDrawn = 0;
	
	gm->curZ++;
	
	for(intptr_t i = 0; w->matches && i < w->matchCnt; i++) {
		if(w->matches[i].excluded) {
			DBG("Match <%s> excluded [%d]\n", w->matches[i].filepath, w->matches[i].excluded);
			continue;
		}
		
		DBG("rendering match: %ld\n", i);
	
		if(lh * linesDrawn > sz.y) break; // stop at the bottom of the window
		
		Vector2 btl_proj = {tl.x + gutter, tl.y + 20 + (lh * linesDrawn)};
		Vector2 btl_file = {btl_proj.x + w->proj_gutter + gutter, tl.y + 20 + (lh * linesDrawn)};
		Vector2 bsz = {sz.x - gutter, (lh)};
		
		if(w->cursorIndex == i) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.selectedItemBgColor;
			GUI_Rect(btl_proj, bsz, color);
		}

		gm->curZ++;
		// the project name
		GUI_TextLine(w->matches[i].projname, strlen(w->matches[i].projname), btl_proj, w->font->name, w->fontsize, &gm->defaults.selectedItemTextColor);
		// the file name
		GUI_TextLine(w->matches[i].filepath, strlen(w->matches[i].filepath), btl_file, w->font->name, w->fontsize, &gm->defaults.selectedItemTextColor);
		gm->curZ--;
		
		linesDrawn++;
	}

}

#include "ui/macros_off.h"




void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd) {

	switch(cmd->cmd) {
		case GUICMD_Calculator_Exit:
			MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
			return 2;
			
		case GUICMD_FuzzyMatcher_MoveCursorV:
			if(w->matchCnt == 0) break;
			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;
			
		case GUICMD_FuzzyMatcher_OpenFile: {
			if(w->matchCnt == 0) break;
			
			int openinplace = w->gs->MainControl_openInPlace;
			open_match(w, w->cursorIndex);
			if(openinplace) return 2;
			break;
		}
		
		default:
			return 1; // command not handled
	}
	
	return 0;
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


GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm, Settings* s, MessagePipe* mp) {

	GUICalculatorControl* w = pcalloc(w);
	
	
//	w->ansalloc = 128;
//	w->answer = calloc(1, sizeof(*w->answer) * w->ansalloc);
	
	
	return w;
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


