#ifndef __gputk_edit_h__
#define __gputk_edit_h__



struct GUIEditUndo;


// single-line only
typedef struct GUIEdit {
	GUIHeader header;
	
	char* buf;
	int buflen;
	int textlen;
	
	int cursorPos; // in characters
	int selEnd; // other side of the selection from the cursor. may be higher or lower.
	
	RING(struct GUIEditUndo*) undo;
	
	char lastTypedClass;
	char numType; // 0 = text, 1 = int, 2 = float
	double numVal;
	
// 	float fontSize;
	float blinkRate;
	float blinkTimer;
	float cursorOffset; // in pixels
	char hasFocus : 1;
	char rightJustify : 1;
	char centerJustify : 1;
	
	
	// offsets, text align
	
} GUIEdit;


GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue);

void GUIEdit_SetText(GUIEdit* w, char* text);
void GUIEdit_SetInt(GUIEdit* w, int64_t ival); 
void GUIEdit_SetDouble(GUIEdit* w, double dval); 
double GUIEdit_GetDouble(GUIEdit* w);
char* GUIEdit_GetText(GUIEdit* w); // returns an internal string. do not cache, do not free.


#endif // __gputk_edit_h__
