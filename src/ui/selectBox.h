#ifndef __EACSMB_ui_selectBox_h__
#define __EACSMB_ui_selectBox_h__





typedef struct GUISelectBoxOption {
	char* label;
	void* data;
	
	int selected : 1;
	int disabled : 1;
	int hovered  : 1;
} GUISelectBoxOption;



struct GUISelectBoxDD;

typedef struct GUISelectBox {
	GUIHeader header;
	
	GUIHeader* dropdownBg;
	GUIHeader* dropdownScrollbar;
	
	int optionCnt;
	int selectedIndex;
	GUISelectBoxOption* options;
	
	int isOpen : 1;
	
} GUISelectBox;




GUISelectBox* GUISelectBox_New(GUIManager* gm);



#endif // __EACSMB_ui_selectBox_h__
