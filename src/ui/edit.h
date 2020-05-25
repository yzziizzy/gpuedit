#ifndef __EACSMB_ui_edit_h__
#define __EACSMB_ui_edit_h__



// onchange
struct GUIEdit;
typedef void (*GUIEditOnChangeFn)(struct GUIEdit*, void*);
typedef void (*GUIEditOnEnterFn)(struct GUIEdit*, void*);


typedef struct GUIEdit {
	GUIHeader header;
	InputEventHandler* inputHandlers;
	
	
	char* buf;
	int buflen;
	int textlen;
	int cursorpos; // in characters
	
	char numType; // 0 = text, 1 = int, 2 = float
	double numVal;
	
// 	float fontSize;
	float blinkRate;
	float cursorOffset; // in pixels
	char hasFocus;
	
	// offsets, text align
	
	GUIEditOnChangeFn onChange;
	void* onChangeData;
	
	GUIEditOnEnterFn onEnter;
	void* onEnterData;
	
} GUIEdit;


GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue);

void GUIEdit_SetText(GUIEdit* ed, char* text);
void GUIEdit_SetInt(GUIEdit* ed, int64_t ival); 
void GUIEdit_SetDouble(GUIEdit* ed, double dval); 
double GUIEdit_GetDouble(GUIEdit* ed);
char* GUIEdit_GetText(GUIEdit* ed); // returns an internal string. do not cache, do not free.


#endif // __EACSMB_ui_edit_h__
