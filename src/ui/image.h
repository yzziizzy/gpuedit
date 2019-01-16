#ifndef __EACSMB_ui_image_h__
#define __EACSMB_ui_image_h__



#include "../pass.h"




typedef struct GUIImage {
	GUIHeader header;
	
	Vector2 offsetNorm;
	Vector2 sizeNorm;
	int texIndex;
	
	GLuint customTexID;
	
} GUIImage;





GUIImage* GUIImage_new(GUIManager* gm, char* name);




typedef struct GUIRenderTarget {
	GUIHeader header;
	
	Vector2i screenRes;
	
	GLuint texID;
	RenderPipeline* rpl;
	
	
} GUIRenderTarget;

GUIRenderTarget* guiRenderTargetNew(Vector2 pos, Vector2 size, RenderPipeline* rpl);

void guiRenderTarget_SetScreenRes(GUIRenderTarget* rt, Vector2i newRes);

// also initialize GUIRenderTarget data
void gui_Image_Init(char* file);

#endif // __EACSMB_ui_image_h__
