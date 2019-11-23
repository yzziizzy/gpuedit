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
	float cursorOffset; // in pixels
	char hasFocus;
	
	// offsets, text align
	
	GUIText* textControl;
	
	GUIEditOnChangeFn onChange;
	void* onChangeData;
	
} GUIEdit;


GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue, Vector2 size);

void GUIEdit_SetText(GUIEdit* ed, char* text);
void GUIEdit_SetInt(GUIEdit* ed, int64_t ival); 
void GUIEdit_SetDouble(GUIEdit* ed, double dval); 
double GUIEdit_GetDouble(GUIEdit* ed);


#endif // __EACSMB_ui_edit_h__
