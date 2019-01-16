#ifndef __EACSMB_ui_edit_h__
#define __EACSMB_ui_edit_h__



// onchange
struct GUIEdit;
typedef void (*GUIEditOnChangeFn)(struct GUIEdit*, void*);


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
	float cursorOffset; // in screen units
	
	// offsets, text align
	
	GUIWindow* bg;
	GUIText* textControl;
	GUIWindow* cursor; // just a thin window
	
	GUIEditOnChangeFn onChange;
	void* onChangeData;
	
} GUIEdit;


GUIEdit* GUIEditNew(char* initialValue, Vector2 pos, Vector2 size);

void guiEditSetText(GUIEdit* ed, char* text);
void guiEditSetInt(GUIEdit* ed, int64_t ival); 
void guiEditSetDouble(GUIEdit* ed, double dval); 
double guiEditGetDouble(GUIEdit* ed);


#endif // __EACSMB_ui_edit_h__
