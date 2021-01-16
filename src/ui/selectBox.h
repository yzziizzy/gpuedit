#ifndef __gputk_selectBox_h__
#define __gputk_selectBox_h__





typedef struct GUISelectBoxOption {
	char* label;
	void* data;
	
	int selected : 1;
	int disabled : 1;
	int hovered  : 1;
} GUISelectBoxOption;




typedef struct GUISelectBox {
	GUIHeader header;
	
	GUIHeader* dropdownBg;
	GUIHeader* dropdownScrollbar;
	float dropdownScrollPos;
	
	int optionCnt;
	int selectedIndex;
	int hoveredIndex;
	GUISelectBoxOption* options;
	
	unsigned int isOpen : 1;
	
} GUISelectBox;




GUISelectBox* GUISelectBox_New(GUIManager* gm);

void GUISelectBox_SetOptions(GUISelectBox* w, GUISelectBoxOption* opts, int cnt);




#endif // __gputk_selectBox_h__
