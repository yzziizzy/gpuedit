#ifndef __EACSMB_gui_internal_h__
#define __EACSMB_gui_internal_h__

// this file is for gui element implementations, not for general outside usage


typedef struct GUIRenderParams {
	Vector2 offset; // parent-defined absolute from the top left of the screen
	Vector2 size; // of the parent's client area that the child lives in
	AABB2 clip; // absolute clipping region
	float baseZ; // accumulated absolute Z from the parent
} GUIRenderParams;



void gui_headerInit(GUIHeader* gh, GUIManager* gm, struct gui_vtbl* vt, struct GUIEventHandler_vtbl* event_vt); 
void gui_defaultUpdatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp);
Vector2 gui_calcPosGrav(GUIHeader* h, GUIRenderParams* grp);
GUIObject* gui_defaultHitTest(GUIHeader* h, Vector2 absTestPos);
Vector2 gui_parent2ChildGrav(GUIHeader* child, GUIHeader* parent, Vector2 pt);



GUIUnifiedVertex* GUIManager_checkElemBuffer(GUIManager* gm, int count);
GUIUnifiedVertex* GUIManager_reserveElements(GUIManager* gm, int count);

void GUIHeader_render(GUIHeader* gh, PassFrameParams* pfp);
void GUIHeader_renderChildren(GUIHeader* gh, PassFrameParams* pfp);

void GUIHeader_updatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp);


static inline AABB2 gui_clipTo(AABB2 parent, AABB2 child) {
	return (AABB2){
		.min.x = fmax(parent.min.x, child.min.x),
		.min.y = fmax(parent.min.y, child.min.y),
		.max.x = fmin(parent.max.x, child.max.x),
		.max.y = fmin(parent.max.y, child.max.y),
	};
}


void gui_drawTextLine(
	GUIManager* gm,
	GUIFont* f,
	float fontSize,
	AABB2 box,  
	struct Color4* color, 
	char* txt, 
	size_t charCount
);


#endif // __EACSMB_gui_internal_h__
