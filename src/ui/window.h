#ifndef __gputk_window_h__
#define __gputk_window_h__



typedef struct GUIWindow {
	GUIHeader header;
	
// 	Vector2 size;
	Vector2 clientSize;
	AABB2 padding;
	
// 	uint32_t color;
	struct Color4 color;
	struct Color4 borderColor;
	float borderWidth;
	
} GUIWindow;




GUIWindow* GUIWindow_New(GUIManager* gm);




#endif // __gputk_window_h__
