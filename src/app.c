
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <math.h>
#include <time.h>


#include <unistd.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "c3dlas/c3dlas.h"
#include "c_json/json.h"
#include "json_gl.h"

#include "mempool.h"

#include "utilities.h"
#include "config.h"
#include "shader.h"
#include "texture.h"
#include "window.h"
#include "app.h"
#include "gui.h"


GLuint proj_ul, view_ul, model_ul;




RenderPipeline* rpipe;


// in renderLoop.c, a temporary factoring before a proper renderer is designed
void drawFrame(XStuff* xs, AppState* as, InputState* is);
void setupFBOs(AppState* as, int resized);


// MapBlock* map;
// TerrainBlock* terrain;




// nothing in here can use opengl at all.
void initApp(XStuff* xs, AppState* as) {
	
	srand((unsigned int)time(NULL));
	
	// this costs 5mb of ram
// 	json_gl_init_lookup();
	
	
// 	TextureAtlas* ta = TextureAtlas_alloc();
// 	ta->width = 256;
// 	TextureAtlas_addFolder(ta, "pre", "assets/ui/icons", 0);
// 	TextureAtlas_finalize(ta);
// 	
	
	as->gui = GUIManager_alloc(&as->globalSettings);
	
	Buffer* buf2 = Buffer_New(as->gui);
	buf2->curCol = 1;
	
	as->currentBuffer = Buffer_New(as->gui);
	as->currentBuffer->curCol = 1;
	
	Buffer_LoadFromFile(as->currentBuffer, "LICENSE");
	Buffer_LoadFromFile(buf2, "config.h");
// 	Buffer_SaveToFile(as->currentBuffer, "test-LICENSE");
	
	
	BufferSelection* sel = pcalloc(sel);
	sel->startLine = as->currentBuffer->first->next->next->next->next->next->next->next->next->next->next->next->next;
	sel->endLine = sel->startLine->next->next->next;
	sel->startCol = 5;
	sel->endCol = 15;
	as->currentBuffer->sel = sel;
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(as->gui);
	gbe->header.size = (Vector2){800, 800}; // TODO update dynamically
	gbe->buffer = as->currentBuffer;
	gbe->font = FontManager_findFont(as->gui->fm, "Courier New");
	gbe->scrollLines = 0;

	GUIBufferEditor* gbe2 = GUIBufferEditor_New(as->gui);
	gbe2->header.size = (Vector2){800, 800}; // TODO update dynamically
	gbe2->buffer = buf2;
	gbe2->font = FontManager_findFont(as->gui->fm, "Courier New");
	gbe2->scrollLines = 0;
	
	TextDrawParams* tdp = pcalloc(tdp);
	tdp->font = gbe->font;
	tdp->fontSize = .5;
	tdp->charWidth = 10;
	tdp->lineHeight = 20;
	tdp->tabWidth = 4;
	
	ThemeDrawParams* theme = pcalloc(theme);
	theme->bgColor =      (struct Color4){ 15,  15,  15, 255};
	theme->textColor =    (struct Color4){240, 240, 240, 255};
	theme->cursorColor =  (struct Color4){255,   0, 255, 180};
	theme->hl_bgColor =   (struct Color4){  0, 200, 200, 255};
	theme->hl_textColor = (struct Color4){250,   0,  50, 255};
	
	BufferDrawParams* bdp = pcalloc(bdp);
	bdp->tdp = tdp;
	bdp->theme = theme;
	bdp->showLineNums = 1;
	bdp->lineNumWidth = 50;
	
	gbe->bdp = bdp;
	gbe2->bdp = bdp;
	
	GUITabControl* tabs = GUITabControl_New(as->gui);
	GUIRegisterObject(tabs, as->gui->root);
	as->tc = tabs;
	
	GUIRegisterObject(gbe2, tabs);
	GUIRegisterObject(gbe, tabs);
	
	GUIManager_pushFocusedObject(as->gui, gbe);
	
	
	
	
	
	as->frameCount = 0;

	
	as->debugMode = 0;

	as->nearClipPlane = .5;
	as->farClipPlane = 1700;

	int ww, wh;
	ww = xs->winAttr.width;
	wh = xs->winAttr.height;
	
	as->screen.wh.x = (float)ww;
	as->screen.wh.y = (float)wh;
	as->gui->screenSize = (Vector2i){ww, wh};
	
	as->screen.aspect = as->screen.wh.x / as->screen.wh.y;
	as->screen.resized = 0;
	

	// set up matrix stacks
	MatrixStack* view, *proj;
	
	view = &as->view;
	proj = &as->proj;
	
	msAlloc(2, view);
	msAlloc(2, proj);

	msIdent(view);
	msIdent(proj);

}

void initAppGL(XStuff* xs, AppState* as) {
	
	
	glerr("left over error on app init");
	
	
	GUIManager_initGL(as->gui, &as->globalSettings);
	as->guiPass = GUIManager_CreateRenderPass(as->gui);
	

	initRenderLoop(as);
	initRenderPipeline();
	

	/*
	getPrintGLEnum(GL_MAX_COLOR_ATTACHMENTS, "meh");
	getPrintGLEnum(GL_MAX_DRAW_BUFFERS, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_WIDTH, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_HEIGHT, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_LAYERS, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_SAMPLES, "meh");
	getPrintGLEnum(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_ARRAY_TEXTURE_LAYERS, "meh");
	getPrintGLEnum(GL_MAX_SAMPLES, "meh");
	getPrintGLEnum(GL_MAX_VERTEX_ATTRIBS, "meh");
	getPrintGLEnum(GL_MIN_PROGRAM_TEXEL_OFFSET, "meh");
	getPrintGLEnum(GL_MAX_PROGRAM_TEXEL_OFFSET, "meh");
	getPrintGLEnum(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, "meh");
	getPrintGLEnum(GL_MAX_UNIFORM_BLOCK_SIZE, "meh");
	getPrintGLEnum(GL_MAX_TEXTURE_SIZE, "meh");
	
	*/
	
	
	
	initTextures();

	
	
	
/*	
	json_file_t* guijsf;
	
	guijsf = json_load_path("assets/config/main_ui.json");
	json_value_t* kids;
	json_obj_get_key(guijsf->root, "children", &kids);
	
	GUICL_LoadChildren(as->gui, as->gui->root, kids);
	
	GUIObject* ps = GUIObject_findChild(as->gui->root, "perfstats");
	gt_terrain = GUIObject_findChild(ps, "terrain");
	gt_solids = GUIObject_findChild(ps, "solids");
	gt_selection = GUIObject_findChild(ps, "selection");
	gt_decals = GUIObject_findChild(ps, "decals");
	gt_emitters = GUIObject_findChild(ps, "emitters");
	gt_effects = GUIObject_findChild(ps, "effects");
	gt_lighting = GUIObject_findChild(ps, "lighting");
	gt_sunShadow = GUIObject_findChild(ps, "sunShadow");
	gt_shading = GUIObject_findChild(ps, "shading");
	gt_gui = GUIObject_findChild(ps, "gui");
	
	
*/

		

}



void preFrame(AppState* as) {

	// update timers
	char frameCounterBuf[128];
	
	static int frameCounter = 0;
	static double last_frame = 0;
	static double lastPoint = 0;
	
	double now;
	
	as->frameTime = now = getCurrentTime();
	
	if (last_frame == 0)
		last_frame = now;
	
	as->frameSpan = (double)(now - last_frame);
	last_frame = now;
	
	frameCounter = (frameCounter + 1) % 60;
	
	
	
	static double sdtime, sseltime, semittime;
	
	if(lastPoint == 0.0f) lastPoint = as->frameTime;
	if(1 /*frameCounter == 0*/) {
		float fps = 60.0f / (as->frameTime - lastPoint);
		
		uint64_t qtime;

#define query_update_gui(qname)		\
		if(!query_queue_try_result(&as->queries.qname, &qtime)) {\
			sdtime = ((double)qtime) / 1000000.0;\
		}\
		snprintf(frameCounterBuf, 128, #qname ":  %.2fms", sdtime);\
		GUIText_setString(gt_##qname, frameCounterBuf);


		//query_update_gui(gui);
		
		lastPoint = now;
	}
	
	

	
}




void postFrame(AppState* as) {
	
	double now;
	
	now = getCurrentTime();
	
	as->perfTimes.draw = now - as->frameTime;
	
}







Vector2i viewWH = {
	.x = 0,
	.y = 0
};
void checkResize(XStuff* xs, AppState* as) {
	if(viewWH.x != xs->winAttr.width || viewWH.y != xs->winAttr.height) {
		
		// TODO: destroy all the textures too
		
		//printf("screen 0 resized\n");
		
		viewWH.x = xs->winAttr.width;
		viewWH.y = xs->winAttr.height;
		
		as->screen.wh.x = (float)xs->winAttr.width;
		as->screen.wh.y = (float)xs->winAttr.height;
		as->gui->screenSize = (Vector2i){xs->winAttr.width, xs->winAttr.height};
		
		as->screen.aspect = as->screen.wh.x / as->screen.wh.y;
		
		as->screen.resized = 1;
		
	}
}



void handleEvent(AppState* as, InputState* is, InputEvent* ev) {
// 	printf("%d %c/* */%d-\n", ev->type, ev->character, ev->keysym);
	
	if(ev->type == EVENT_KEYUP && is->keyState[64]) {
		if(ev->keysym == XK_Right) {
			GUITabControl_NextTab(as->tc, 1);
			return;
		}
		else if(ev->keysym == XK_Left) {
			GUITabControl_PrevTab(as->tc, 1);
			return;
		}
	}
	
	
	switch(ev->type) {
		case EVENT_KEYUP:
		case EVENT_KEYDOWN:
			GUIManager_HandleKeyInput(as->gui, is, ev);
			break;
		case EVENT_MOUSEUP:
		case EVENT_MOUSEDOWN:
			GUIManager_HandleMouseClick(as->gui, is, ev);
			break;
		case EVENT_MOUSEMOVE:
			GUIManager_HandleMouseMove(as->gui, is, ev);
			break;
	}
}


void prefilterEvent(AppState* as, InputState* is, InputEvent* ev) {
	// drags, etc
	
	// TODO: fix; passthrough atm
	handleEvent(as, is, ev);
	
}




#define PF_START(x) as->perfTimes.x = getCurrentTime()
#define PF_STOP(x) as->perfTimes.x = timeSince(as->perfTimes.x)

void appLoop(XStuff* xs, AppState* as, InputState* is) {
	
	// main running loop
	while(1) {
		InputEvent iev;
		
		for(int i = 0; i < 1000; i++) {
			int drawRequired = 0;
			if(processEvents(xs, is, &iev, -1)) {
	// 			// handle the event
				prefilterEvent(as, is, &iev);
			}
			
			
			if(drawRequired) break;
		}
		
		
		checkResize(xs, as);
		
		preFrame(as); // updates timers
		
		drawFrame(xs, as, is);
		
		as->screen.resized = 0;
		
		postFrame(as); // finishes frame-draw timer
	
		
		if(as->frameSpan < 1.0/15.0) {
			// shitty estimation based on my machine's heuristics, needs improvement
			float sleeptime = (((1.0/15.0) * 1000000) - (as->frameSpan * 1000000)) * 1.7;
			//printf("sleeptime: %f\n", sleeptime / 1000000);
			//sleeptime = 1000;
			if(sleeptime > 0) usleep(sleeptime); // problem... something is wrong in the math
		}
// 		sleep(1);
	}
}
