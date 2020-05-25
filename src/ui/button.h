#ifndef __EACSMB_ui_button_h__
#define __EACSMB_ui_button_h__


typedef struct GUIButton {
	GUIHeader header;
	
	char* label;
	
	int isHovered : 1;
	
} GUIButton;



GUIText* GUIButton_New(GUIManager* gm, char* str);




#endif // __EACSMB_ui_button_h__
