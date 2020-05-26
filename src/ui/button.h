#ifndef __EACSMB_ui_button_h__
#define __EACSMB_ui_button_h__



// Note: listen on the parent object for clicks


typedef struct GUIButton {
	GUIHeader header;
	
	char* label;
	
	int isHovered : 1;
	int isDisabled : 1;
	
} GUIButton;



GUIText* GUIButton_New(GUIManager* gm, char* str);




#endif // __EACSMB_ui_button_h__
