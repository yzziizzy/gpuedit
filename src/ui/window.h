#ifndef __EACSMB_ui_window_h__
#define __EACSMB_ui_window_h__



typedef struct GUIWindow {
	GUIHeader header;
	
// 	Vector2 size;
	Vector2 clientSize;
	AABB2 padding;
	
// 	uint32_t color;
	Vector color;
	Vector4 borderColor;
	float borderWidth;
	float fadeWidth;
	
} GUIWindow;




GUIWindow* GUIWindow_new(GUIManager* gm);




#endif // __EACSMB_ui_window_h__
