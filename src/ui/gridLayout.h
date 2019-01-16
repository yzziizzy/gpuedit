#ifndef __EACSMB_ui_gridLayout_h__
#define __EACSMB_ui_gridLayout_h__






typedef struct GUIGridLayout {
	GUIHeader header;
	
	int maxRows;
	int maxCols;
	
	Vector2 spacing;
	
} GUIGridLayout;



GUIGridLayout* GUIGridLayout_new(GUIManager* gm, Vector2 pos, Vector2 spacing);




#endif // __EACSMB_ui_gridLayout_h__
