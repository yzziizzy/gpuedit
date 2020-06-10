
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>w
#include <math.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <termios.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "c3dlas/c3dlas.h"
#include "c_json/json.h"
#include "json_gl.h"

#include "sti/sti.h"

#include "utilities.h"
#include "config.h"
#include "shader.h"
#include "texture.h"
#include "window.h"
#include "app.h"
#include "gui.h"

// temp
#include "highlighters/c.h"


GLuint proj_ul, view_ul, model_ul;


int g_DisableSave = 0; // debug flag to disable saving

RenderPipeline* rpipe;


// in renderLoop.c, a temporary factoring before a proper renderer is designed
void drawFrame(XStuff* xs, AppState* as, InputState* is);
void setupFBOs(AppState* as, int resized);


// MapBlock* map;
// TerrainBlock* terrain;

void resize_callback(XStuff* xs, void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
	
	GUIEvent gev = {
		.type = GUIEVENT_ParentResize,
		.size = {.x = xs->winSize.x, .y = xs->winSize.y},
		.originalTarget = gm->root,
	};
	
	GUIObject_TriggerEvent(gm->root, &gev);
}

static struct child_process_info* cc;

// nothing in here can use opengl at all.
void initApp(XStuff* xs, AppState* as, int argc, char* argv[]) {
	
	srand((unsigned int)time(NULL));
	
	char* args[] = {
		"/bin/bash",
		"-i",
		"-l",
		NULL,
	};
	
	as->commands = CommandList_loadFile("./config/commands.txt");
	
// 	cc = AppState_ExecProcessPipe(NULL, "/bin/bash", args);
	
	// this costs 5mb of ram
// 	json_gl_init_lookup();
	
	
	as->ta = TextureAtlas_alloc(&as->globalSettings);
	as->ta->width = 32;
	TextureAtlas_addFolder(as->ta, "icon", "images", 0);
	TextureAtlas_finalize(as->ta);
// 	
	
	/*
	Highlighter* ch = pcalloc(ch);
	initCStyles(ch);
	Highlighter_PrintStyles(ch);
	Highlighter_LoadStyles(ch, "config/c_colors.txt");
	*/
	
	as->gui = GUIManager_alloc(&as->globalSettings);
	as->gui->ta = as->ta;
	xs->onResize = resize_callback;
	xs->onResizeData = as->gui;
	as->gui->defaults.tabBorderColor = COLOR4_FROM_HEX(120,120,120,255);
	as->gui->defaults.tabActiveBgColor = COLOR4_FROM_HEX(80,80,80,255);
	as->gui->defaults.tabHoverBgColor = COLOR4_FROM_HEX(40,40,40,255);
	as->gui->defaults.tabBgColor = COLOR4_FROM_HEX(10,10,10,255);
	as->gui->defaults.tabTextColor = COLOR4_FROM_HEX(200,200,200,255);
	
	as->gui->windowTitleSetFn = XStuff_SetWindowTitle;
	as->gui->windowTitleSetData = xs;
	
	as->gui->mouseCursorSetFn = XStuff_SetMouseCursor;
	as->gui->mouseCursorSetData = xs;
	
	as->mc = GUIMainControl_New(as->gui, &as->globalSettings);
	as->mc->as = as;
	as->mc->commands = as->commands;
	GUIRegisterObject(as->gui->root, as->mc);

	
	
	// command line args
	for(int i = 1; i < argc; i++) {
		char* a = argv[i];
		
		// for debugging
		if(0 == strcmp(a, "--disable-save")) {
			printf("Buffer saving disabled.\n");
			g_DisableSave = 1;
		}
		
		
		// look for files to load in arguments
		// -f works too
		if(a[0] == '-') {
			if(a[1] == 'f' && a[2] == NULL) {
				i++;
				if(i < argc) {
					GUIMainControl_LoadFile(as->mc, argv[i]);
				}
			}
			
			continue;
		}
		
		GUIMainControl_LoadFile(as->mc, a);
	}
	
	
	// for debugging
	GUIMainControl_LoadFile(as->mc, "testfile.h");
	GUIMainControl_LoadFile(as->mc, "testfile.c");
// 	GUIMainControl_LoadFile(as->mc, "src/buffer.c");
// 	GUIMainControl_LoadFile(as->mc, "src/bufferEditor.c");
	
	GUIMainControl_OpenFileBrowser(as->mc, "./");
	
	GUIManager_pushFocusedObject(as->gui, as->mc);
	
	
	
	
	
	as->frameCount = 0;
	
	as->debugMode = 0;
	
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
	
	TextureAtlas_initGL(as->ta, &as->globalSettings);
	
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


// effectively a better, asynchronous version of system()
void AppState_ExecProcess(AppState* as, char* execPath, char* args[]) {
	
	
	int childPID = fork();
	
	if(childPID == -1) {
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return;
	}
	else if(childPID == 0) { // child process
		
		execvp(execPath, args); // never returns if successful
		
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		// TODO: put the pid and info into an array somewhere
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
	}
}

// http://git.suckless.org/st/file/st.c.html#l786

// effectively a better, asynchronous version of system()
// redirects and captures the child process i/o
struct child_pty_info* AppState_ExecProcessPTY(AppState* as, char* execPath, char* args[]) {
	
	int master, slave; // pty
	
	
	errno = 0;
	if(openpty(&master, &slave, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "Error opening new pty for '%s' [errno=%d]\n", execPath, errno);
		return NULL;
	}
	
	errno = 0;
	
	int childPID = fork();
	if(childPID == -1) {
		
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return NULL;
	}
	else if(childPID == 0) { // child process
		
		setsid();
		// redirect standard fd's to the pipe fd's 
		if(dup2(slave, fileno(stdin)) == -1) {
			printf("failed 1\n");
			exit(errno);
		}
		if(dup2(slave, fileno(stdout)) == -1) {
			printf("failed 2\n");
			exit(errno);
		}
		if(dup2(slave, fileno(stderr)) == -1) {
			printf("failed 3\n");
			exit(errno);
		}
		
		if(ioctl(slave, TIOCSCTTY, NULL) < 0) {
			fprintf(stderr, "ioctl TIOCSCTTY failed: %s, %d\n", execPath, errno);
		}
		
		// close original fd's
		close(master);
		close(slave);
		
		// die when the parent does (linux only)
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		
		// swap for the desired program
		execvp(execPath, args); // never returns if successful
		 
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		
		// close the child-end of the pipes
		struct child_pty_info* cpi;
		cpi = calloc(1, sizeof(*cpi));
		cpi->pid = childPID;
		cpi->pty = master;
		
		// set to non-blocking
		fcntl(master, F_SETFL, fcntl(master, F_GETFL) | FNDELAY | O_NONBLOCK);
		
		close(slave);
		
// 		tcsetattr(STDIN_FILENO, TCSANOW, &master);
// 		fcntl(master, F_SETFL, FNDELAY);
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
	
}


// effectively a better, asynchronous version of system()
// redirects and captures the child process i/o
struct child_process_info* AppState_ExecProcessPipe(AppState* as, char* execPath, char* args[]) {
	
	int master, slave; //pty
	int in[2]; // io pipes
	int out[2];
	int err[2];
	
	const int RE = 0;
	const int WR = 1;
	
	// 0 = read, 1 = write
	
	if(pipe(in) < 0) {
		return NULL;
	}
	if(pipe(out) < 0) {
		close(in[0]);
		close(in[1]);
		return NULL;
	}
	if(pipe(err) < 0) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		return NULL;
	}
	
	errno = 0;
	if(openpty(&master, &slave, NULL, NULL, NULL) < 0) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		fprintf(stderr, "Error opening new pty for '%s' [errno=%d]\n", execPath, errno);
		return NULL;
	}
	
	errno = 0;
	
	int childPID = fork();
	if(childPID == -1) {
		
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return NULL;
	}
	else if(childPID == 0) { // child process
		
		// redirect standard fd's to the pipe fd's 
		if(dup2(in[RE], fileno(stdin)) == -1) {
			printf("failed 1\n");
			exit(errno);
		}
		if(dup2(out[WR], fileno(stdout)) == -1) {
			printf("failed 2\n");
			exit(errno);
		}
		if(dup2(err[WR], fileno(stderr)) == -1) {
			printf("failed 3\n");
			exit(errno);
		}
		
		// close original fd's used by the parent
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		close(err[0]);
		close(err[1]);
		
		close(master);
		close(slave);
		
		// die when the parent does (linux only)
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		
		// swap for the desired program
		execvp(execPath, args); // never returns if successful
		
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		
		// close the child-end of the pipes
		struct child_process_info* cpi;
		cpi = calloc(1, sizeof(*cpi));
		
		cpi->child_stdin = in[WR];
		cpi->child_stdout = out[RE];
		cpi->child_stderr = err[RE];
		cpi->f_stdin = fdopen(cpi->child_stdin, "wb");
		cpi->f_stdout = fdopen(cpi->child_stdout, "rb");
		cpi->f_stderr = fdopen(cpi->child_stderr, "rb");
		
		// set to non-blocking
		fcntl(cpi->child_stdout, F_SETFL, fcntl(cpi->child_stdout, F_GETFL) | O_NONBLOCK);
		fcntl(cpi->child_stderr, F_SETFL, fcntl(cpi->child_stderr, F_GETFL) | O_NONBLOCK);
		
		close(in[0]);
		close(out[1]); 
		close(err[1]); 
		
		close(slave);
		
		cpi->pid = childPID;
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
}


void AppState_UpdateSettings(AppState* as, GlobalSettings* gs) {
	
	as->globalSettings = *gs;
	
	GUIMainControl_UpdateSettings(as->mc, gs);
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
	
	GUIManager_Reap(as->gui);
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
		/*
		char buffer[1024];
		
		
		errno = 0;
// 		int len = read(buffer, 5, cc->child_stdout);
		int len = read(cc->child_stdout, buffer, 1023);
		if(len == -1 && errno != EWOULDBLOCK) {
			printf("1: %d %s\n", errno,  strerror(errno));
		}
		
		if(len > 0) {
			buffer[len] = 0;
			printf("from bash[%d]: '%.*s'\n", len, len, buffer);
		}
		
		errno = 0;
		len = read(cc->child_stderr, buffer, 1023);
		if(len == -1 && errno != EWOULDBLOCK) {
			printf("2: %d %s\n", errno, strerror(errno));
		}
		if(len > 0) {
			buffer[len] = 0;
			printf("from bash[%d]: '%.*s'\n", len, len, buffer);
		}
		*/
		
		checkResize(xs, as);
		
// 		double now = getCurrentTime();
		preFrame(as); // updates timers
		
		drawFrame(xs, as, is);
		
		as->screen.resized = 0;
		
		postFrame(as); // finishes frame-draw timer
// 		printf("frame time: %fms\n", timeSince(now) * 1000.0);
		
		if(as->frameSpan < 1.0/60.0) {
			// shitty estimation based on my machine's heuristics, needs improvement
			float sleeptime = (((1.0/60.0) * 1000000) - (as->frameSpan * 1000000)) * 1.7;
			//printf("sleeptime: %f\n", sleeptime / 1000000);
			//sleeptime = 1000;
			if(sleeptime > 0) usleep(sleeptime); // problem... something is wrong in the math
		}
// 		sleep(1);
	}
}

