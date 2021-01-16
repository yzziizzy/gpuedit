#ifndef __gputk_scrollWindow_h__
#define __gputk_scrollWindow_h__



typedef struct GUIScrollWindow {
	GUIHeader header;
	
	float sbWidth;
	Vector2 internalSize; // the extent of the child windows internally
	
	Vector2 scrollPos;// in pixels
	
} GUIScrollWindow;




GUIScrollWindow* GUIScrollWindow_new(GUIManager* gm);




#endif // __gputk_scrollWindow_h__
