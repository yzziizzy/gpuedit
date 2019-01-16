
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "../gui.h"



static void render(GUIImageButton* ib, GameState* gs, PassFrameParams* pfp) {
	guiRender(ib->img, gs, pfp);
}

static void delete(GUIImageButton* ib) {

}


GUIImageButton* guiImageButtonNew(Vector2 pos, Vector2 size, float zIndex, char* imgName) {
	
	GUIImageButton* imb;
	
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Delete = delete
	};
	
	static InputEventHandler input_vt = {
		//.keyText = recieveText,
		//.keyDown = keyDown,
	};
	
	im = calloc(1, sizeof(*im));
	CHECK_OOM(im);
	
	guiHeaderInit(&im->header);
	im->header.vt = &static_vt;
	im->inputHandlers = &input_vt;
		
	im->header.hitbox.min.x = pos.x;
	im->header.hitbox.min.y = pos.y;
	im->header.hitbox.max.x = pos.x + size.x;
	im->header.hitbox.max.y = pos.y + size.y;
	
	im->header.topleft = pos;
	im->header.size = size;
	im->header.z = zIndex;
	
	
	im->imgName = imgName;
	// TODO: look up name by index
	//im->imgIndex = imgIndex;
	
	im->img = guiImageNew(pos, size, 1, im->imgIndex);
	guiRegisterObject(im->img, &im->header);
	
	//ed->textControl = guiTextNew(initialValue, pos, 6.0f, "Arial");
	//ed->textControl->header.size.x = .5;
	//guiRegisterObject(ed->textControl, &ed->bg->header);
	
	
	return im;
}









