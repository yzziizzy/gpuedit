
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <time.h>



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


static void main_drag_handler(InputEvent* ev, AppState* as);
static void main_key_handler(InputEvent* ev, AppState* as);
static void main_perframe_handler(InputState* is, float frameSpan, AppState* as);
static void main_click_handler(InputEvent* ev, AppState* as);
static void main_move_handler(InputEvent* ev, AppState* as);


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
	
	
	as->currentBuffer = Buffer_New(as->gui);
	as->currentBuffer->curLine = 1;
	as->currentBuffer->curCol = 1;
	as->currentBuffer->font = FontManager_findFont(as->gui->fm, "Courier New");

	Buffer_AddLineBelow(as->currentBuffer);
	Buffer_AdvanceLines(as->currentBuffer, 1);
	Buffer_insertText(as->currentBuffer, "foobar1", 0);
	test(as->currentBuffer);
	
	Buffer_AddLineBelow(as->currentBuffer);
	Buffer_AdvanceLines(as->currentBuffer, 1);
	Buffer_insertText(as->currentBuffer, "foobar2", 0);
	test(as->currentBuffer);
	
	Buffer_AddLineBelow(as->currentBuffer);
	Buffer_AdvanceLines(as->currentBuffer, 1);
	Buffer_insertText(as->currentBuffer, "foobar3", 0);
	test(as->currentBuffer);

	Buffer_AppendLine(as->currentBuffer, "foobar4", 0);
	Buffer_AppendLine(as->currentBuffer, "\tfoobar5", 0);
	Buffer_AppendLine(as->currentBuffer, " foobar6", 0);
	Buffer_AppendLine(as->currentBuffer, "longggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg!", 0);
	
	Buffer_loadRawText(as->currentBuffer, "first\nabc\nabc\nabc\nabc\nlast", 0);
	
	GUIRegisterObject(as->currentBuffer, as->gui->root);
	
	// input handlers
	as->defaultInputHandlers = calloc(1, sizeof(*as->defaultInputHandlers));
	as->defaultInputHandlers->dragStop = (void*)main_drag_handler;
	as->defaultInputHandlers->keyUp = (void*)main_key_handler;
	as->defaultInputHandlers->perFrame = (void*)main_perframe_handler;
	as->defaultInputHandlers->click = (void*)main_click_handler;
	as->defaultInputHandlers->mouseMove = (void*)main_move_handler;
	InputFocusStack_PushTarget(&as->ifs, as, defaultInputHandlers);
	
	
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



static void main_perframe_handler(InputState* is, float frameSpan, AppState* as) {
	
}



static void main_drag_handler(InputEvent* ev, AppState* as) {
	

	
}

static void main_key_handler(InputEvent* ev, AppState* as) {
	
	if(ev->character == 'c') {
		exit(0);
	}
	
	if(ev->keysym == XK_Delete) {

	}
	
	if(ev->keysym == XK_Insert) {
// 		GUIText_setString(gtSelectionDisabled, as->selectionPassDisabled ? "Selection Disabled" : "");
	}

	
	if(ev->character == 'k') {



		
	}
}  


static void main_click_handler(InputEvent* ev, AppState* as) {

	if(ev->button == 1) {
		
		// BUG: used inverse cursor pos. changed to compile temporarily
		GUIObject* hit;
		
		Vector2 pos = (Vector2){ev->intPos.x, as->screen.wh.y - ev->intPos.y};
// 		hit = GUIManager_hitTest(as->gui, pos);
		
		hit = GUIManager_triggerClick(as->gui, pos);
		
		
		printf("\n\n----> %f, %f \n", ev->normPos.x, ev->normPos.y);
		if(hit) {
			printf("@@clicked in window %p %p  %f,%f\n", as->gui->root, hit, hit->header.size.x, hit->header.size.y);
		}
		else {
		}
	}
}




static void main_move_handler(InputEvent* ev, AppState* as) {
	

}


void updateView(XStuff* xs, AppState* as, InputState* is) {

	//printf("sun theta %f\n", as->sunTheta);
	
	msPush(&as->proj);
// 	msPerspective(60, as->screen.aspect, as->nearClipPlane, as->farClipPlane, &as->proj);
	msOrtho(-1, 1, -1, 1, 0, 1000, &as->proj);
// 	printf("%f, %f, %f\n", as->screen.aspect, as->nearClipPlane, as->farClipPlane);
	
	msPush(&as->view);
	
	msIdent(&as->view);
	// order matters! don't mess with this.
// 	msTrans3f(0, -1, as->zoom, &as->view);
// 	msRot3f(1, 0, 0, F_PI / 6, &as->view);
// 	msRot3f(0,1,0, as->direction, &as->view);
// 	msTrans3f(-as->lookCenter.x, 0, -as->lookCenter.y, &as->view);
	
	
	// y-up to z-up rotation
// 	msRot3f(1, 0, 0, F_PI_2, &as->view);
// 	msScale3f(1, 1, -1, &as->view);
	
	// calculate cursor position
	Vector cursorp;
	Vector eyeCoord;
	Vector worldCoord;
	Matrix p, invp, invv;
	
	// device space (-1:1)
	Vector devCoord;
	devCoord.x = 0.50;
	devCoord.y = 0.50;
	devCoord.z = -1.0;
	
	// eye space
	mInverse(msGetTop(&as->proj), &invp);
	// 	vMatrixMul(&devCoord, &invp, &eyeCoord);
// 	vNorm(&eyeCoord, &eyeCoord);
	
	// world space
	mInverse(msGetTop(&as->view), &invv);
	// 	vMatrixMul(&eyeCoord, &invv, &worldCoord);
// 	vNorm(&worldCoord, &worldCoord);
	
	mFastMul(&invp, &invv, &as->mProjWorld);
	
	Vector zero = {0,0,0};
	vMatrixMul(&zero, &invv, &as->eyePos);
	
}




// deprecated, use above
void checkCursor(AppState* as, InputState* is) {
	
	union {
		unsigned char rgb[4];
		uint32_t in;
	} u;
	glexit("pre selection buff");
	

	
	int w = (int)as->screen.wh.x;
	int h = (int)as->screen.wh.y;
	
	int x = (int)is->lastCursorPosPixels.x;
	int y = (int)is->lastCursorPosPixels.y;

	
	// wove a window with the cursor
	//gw_test->header.topleft = (Vector2){x, h - y};
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





#define PF_START(x) as->perfTimes.x = getCurrentTime()
#define PF_STOP(x) as->perfTimes.x = timeSince(as->perfTimes.x)

void appLoop(XStuff* xs, AppState* as, InputState* is) {
	as->frameCount++;
	
// 	printf("-----------frame------------\n");
	

	checkResize(xs,as);
	
		PF_START(preframe);
	preFrame(as);
		PF_STOP(preframe);
	
	InputFocusStack_DispatchPerFrame(&as->ifs, is, as->frameSpan);
	
	updateView(xs, as, is);
	
	checkCursor(as, is);
	
	
	drawFrame(xs, as, is);
	
	
	as->screen.resized = 0;

	postFrame(as);
// 	printf("^^^^^^^^^^frame^^^^^^^^^^\n");
}
