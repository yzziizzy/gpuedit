#ifndef __EACSMB_ui_imgButton_h__
#define __EACSMB_ui_imgButton_h__



typedef struct GUIImageButton {
	GUIHeader header;
	
	float border;
	struct Color4 normalColor;
	struct Color4 hoverColor;
	struct Color4 activeColor;
// 	int imgIndex;
	
	char active;
	char hovered;
	
	//GUIWindow* bg;
	GUIImage* img;
	
} GUIImageButton;


GUIImageButton* GUIImageButton_New(GUIManager* gm, float border, char* imgName);




#endif // __EACSMB_ui_imgButton_h__
