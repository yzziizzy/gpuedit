
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "../gui.h"
#include "../gui_internal.h"



static void render(GUIImageButton* w, PassFrameParams* pfp) {
	
	w->hovered = 0;
	Vector2i mp = {0,0};//w->header.gm->mousePosPx; 
	Vector2 tl = w->header.absTopLeft;

	if(mp.x >= tl.x && mp.x <= tl.x + w->header.size.x &&
		mp.y >= tl.y && mp.y <= tl.y + w->header.size.y) {
		w->hovered = 1;
	}
	
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 1);
	
	*v = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = w->active ? w-> activeColor : (w->hovered ? w->hoverColor : w->normalColor), // TODO: color
		
		.z = w->header.z,
		.alpha = w->header.alpha,
	};
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}

static void delete(GUIImageButton* ib) {

}
static void resize(GUIImageButton* w, Vector2 newSz) {
	guiResize(&w->img->header, (Vector2){newSz.x - w->border*2, newSz.y - w->border*2});
}

// static void mouseEnter(InputEvent* ev, GUIImageButton* w) {
// 	w->hovered = 1;
// 	printf("entered \n");
// }
// static void mouseLeave(InputEvent* ev, GUIImageButton* w) {
// 	w->hovered = 0;
// 	printf("left \n");
// }

static int click(GUIImageButton* w, Vector2 clickPos) {
	GUIImageButton* w2 = w->header.parent;
	w2->active = !w2->active;
	return 0;
}



GUIImageButton* GUIImageButton_New(GUIManager* gm, float border, char* imgName) {
	
	GUIImageButton* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Resize = (void*)resize,
	};
	
	static InputEventHandler input_vt = {
		//.keyText = recieveText,
		//.keyDown = keyDown,
// 		.mouseEnter = (input_event_fn)mouseEnter,
// 		.mouseLeave = (input_event_fn)mouseLeave,
	};
	
	w = calloc(1, sizeof(*w));
	CHECK_OOM(w);
	
	gui_headerInit(&w->header, gm, &static_vt);
// 	w->header.input_vt = &input_vt;
	
// 	w->header.z = zIndex;
	w->border = border;
	
	w->img = GUIImage_new(gm, imgName);
	GUIRegisterObject(w->img, &w->header);
// 	w->header.onClick = (GUI_OnClickFn)click;
	w->img->header.topleft = (Vector2){w->border, w->border};
	w->img->header.onClick = (GUI_OnClickFn)click;
	
	w->normalColor = (struct Color4){0, 0, 255, 64};
	w->hoverColor = (struct Color4){0, 0, 255, 238};
	w->activeColor = (struct Color4){0, 255, 0, 230};
	
	return w;
}









