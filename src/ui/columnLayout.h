#ifndef __gputk_columnLayout_h__
#define __gputk_columnLayout_h__






typedef struct GUIColumnLayout {
	GUIHeader header;
	
// 	GUIWindow* bg; // optional
	
	float spacing;
	
	
} GUIColumnLayout;

GUIColumnLayout* GUIColumnLayout_new(GUIManager* gm, Vector2 pos, float spacing, float zIndex);




#endif // __gputk_columnLayout_h__
