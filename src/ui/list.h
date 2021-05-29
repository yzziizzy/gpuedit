#ifndef __gputk_list_h__
#define __gputk_list_h__


#define GUI_LIST_FROM_BOTTOM 0x00010000
#define GUI_LIST_AUTOSCROLL  0x00020000


struct GUIStringListLink {
	char* s;
	struct GUIStringListLink* next, *prev;
};


typedef struct GUIStringList {
	GUIHeader header;
	
	struct GUIStringListLink* first, *last;
	int itemCnt;
	int maxItems;
	
	float scrollPos;
	
	

} GUIStringList;





GUIStringList* GUIStringList_New(GUIManager* gm);


void GUIStringList_PrependItem(GUIStringList* w, char* label);
void GUIStringList_AddItem(GUIStringList* w, char* label);


#endif // __gputk_list_h__
