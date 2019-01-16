#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "utilities.h"
#include "c3dlas/c3dlas.h"
#include "window.h"



// lag between the X server and the game's internal timekeeping, for adjusting events 
static double game_server_diff = 0;

void initGLEW();

 
 
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;


const int context_attr[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
	GLX_CONTEXT_MINOR_VERSION_ARB, 5,
#ifdef USE_KHR_DEBUG
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
	None
};


const int visual_attr[] = {
	GLX_X_RENDERABLE    , True,
	GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
	GLX_RENDER_TYPE     , GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
	GLX_RED_SIZE        , 8,
	GLX_GREEN_SIZE      , 8,
	GLX_BLUE_SIZE       , 8,
	GLX_ALPHA_SIZE      , 8,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	GLX_DOUBLEBUFFER    , True,
	//GLX_SAMPLE_BUFFERS  , 1, // antialiasing
	//GLX_SAMPLES         , 4,
	None
};
	


XErrorEvent* xLastError = NULL;
char xLastErrorStr[1024] = {0};

int xErrorHandler(Display *dpy, XErrorEvent *ev) {
	
	xLastError = ev;
	XGetErrorText(dpy, ev->error_code, xLastErrorStr, 1024);
	
	fprintf(stderr, "X Error %d: %s", ev->error_code, xLastErrorStr);
	
    return 0;
}



double GameTimeFromXTime(Time t) {
	return ((double)t / 1000.0) - game_server_diff;
}

// for motion events, extracts the state of meta keys
unsigned char TranslateModState(unsigned int state) {
	unsigned short out = 0;
	
	if(state & ShiftMask) out |= IS_SHIFT; 
	if(state & ControlMask) out |= IS_CONTROL; 
	if(state & Mod1Mask) out |= IS_ALT; 
	if(state & Mod4Mask) out |= IS_TUX; 
	
	return out;
}


void _khr_debug_callback( // i hate this stype of formatting, but this function has too many damn arguments
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	GLvoid *userParam) {

	printf(TERM_BOLD TERM_COLOR_RED "GL ERROR:" TERM_RESET TERM_COLOR_RED " %s\n" TERM_RESET, message);
	
}



 
// this function will exit() on fatal errors. what good is error handling then?
int initXWindow(XStuff* xs) {
	
	GLXFBConfig* fbconfigs;
	GLXFBConfig chosenFBC;
	int fbcount, i;
	int best_fbc = -1, best_num_samp = -1;
	XSetWindowAttributes setWinAttr;
	
	// for the empty cursor
	Pixmap emptyPx;
	XColor black = {0, 0, 0};
	static char zeros[] = {0};
	
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	
	
	xs->display = XOpenDisplay(NULL);
	if(xs->display == NULL) {
		printf("Cannot connect to X server\n");
		exit(1);
	}

	xs->rootWin = DefaultRootWindow(xs->display);
	
	
	// find a framebuffer config
	fbconfigs = glXChooseFBConfig(xs->display, DefaultScreen(xs->display), visual_attr, &fbcount);
	if(!fbconfigs) {
		fprintf(stderr, "No usable framebuffer config\n" );
		exit(1);
	}
	
	
	// try to get the requested MSAA, or the closest to it without going over
	for(i = 0; i < fbcount; i++) {
		XVisualInfo* vi;
		int samp_buf, samples;
		
		vi = glXGetVisualFromFBConfig(xs->display, fbconfigs[i]);
		if(!vi) continue;
			
		glXGetFBConfigAttrib(xs->display, fbconfigs[i], GLX_SAMPLE_BUFFERS, &samp_buf);
		glXGetFBConfigAttrib(xs->display, fbconfigs[i], GLX_SAMPLES, &samples);
		glerr("samples");
		
		if(best_fbc < 0 || samp_buf && samples > best_num_samp && samples <= xs->targetMSAA) {
			best_fbc = i;
			best_num_samp = samples;
		}
		
		XFree(vi);
	}
	
	chosenFBC = fbconfigs[best_fbc];
	
	XFree(fbconfigs);
	
	
	// Get a visual
	xs->vi = glXGetVisualFromFBConfig(xs->display, chosenFBC);
	
	
	xs->colorMap = XCreateColormap(xs->display, xs->rootWin, xs->vi->visual, AllocNone);
	setWinAttr.colormap = xs->colorMap;
	setWinAttr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | PropertyChangeMask;

	xs->clientWin = XCreateWindow(xs->display, xs->rootWin, 0, 0, 800, 800, 0, xs->vi->depth, InputOutput, xs->vi->visual, CWColormap | CWEventMask, &setWinAttr);

	XMapWindow(xs->display, xs->clientWin);
	
	XStoreName(xs->display, xs->clientWin, xs->windowTitle);
	
	// figure out the X server's time
	XEvent xev;
	while(XNextEvent(xs->display, &xev)) {
		double gametime;
		double servertime;
		if(xev.type == PropertyNotify) {
			gametime = getCurrentTime();
			servertime = (double)xev.xproperty.time / 1000.0;
			game_server_diff = gametime - servertime;
		}
	}
	
	// don't check for supported extensions, just fail hard and fast if the computer is a piece of shit.
	
	// cause putting now-ubiquitous stuff in headers is lame...
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
	if(!glXCreateContextAttribsARB) {
		fprintf(stderr, "glXCreateContextAttribsARB() not found. Upgrade your computer.\n" );
		exit(1);
	}
	
	XSetErrorHandler(&xErrorHandler);
	
	xs->glctx = glXCreateContextAttribsARB(xs->display, chosenFBC, 0, True, context_attr);
	
	
	// squeeze out any errors
	XSync(xs->display, False);

	glXMakeCurrent(xs->display, xs->clientWin, xs->glctx);
	
	
	// have to have a current GLX context before initializing GLEW
	initGLEW();
	
#ifdef USE_KHR_DEBUG
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	
	glDebugMessageCallback(_khr_debug_callback , NULL);
#endif
	
	
	// set up the empty cursor used to hide the cursor
	emptyPx = XCreateBitmapFromData(xs->display, xs->clientWin, zeros, 1, 1);
	xs->noCursor = XCreatePixmapCursor(xs->display, emptyPx, emptyPx, &black, &black, 0, 0);
	XFreePixmap(xs->display, emptyPx);
}




#define CLAMP(min, mid, max) MIN(max, MAX(mid, min))



void processEvents(XStuff* xs, InputState* st, InputFocusStack* ifs, int max_events) {
	
	XEvent xev;
	int evcnt;
	int x, y;
	float fx, fy;

	int rootX, rootY, clientX, clientY;
	Window rootReturn, clientReturn;
	unsigned int mouseMask;
	
	Vector2i pixelPos;
	Vector2 normPos;
	Vector2 invNormPos;

	InputEvent iev;
	
	iev.is = st;
	
	if(max_events <= 0) max_events = INT_MAX;
	
	// BUG: preserve the held states of things?
	//clearInputState(st);
	
// 	if(XQueryPointer(xs->display, xs->clientWin, &rootReturn, &clientReturn, &rootX, &rootY, &clientX, &clientY, &mouseMask)) {
// 		if(xs->winAttr.height > 0 && xs->winAttr.width > 0) { // make sure the window is initialized
// 			
// 			pixelPos.x = CLAMP(0, clientX, xs->winAttr.width);
// 			pixelPos.y = CLAMP(0, xs->winAttr.height - clientY, xs->winAttr.height);
// 			
// 			// it seems the inverse is more correct
// 			normPos.x = (float)clientX / (float)xs->winAttr.width;
// 			normPos.y = 1.0 - ((float)clientY / (float)xs->winAttr.height); // opengl is inverted to X
// 			invNormPos.x = (float)clientX / (float)xs->winAttr.width;
// 			invNormPos.y = (float)clientY / (float)xs->winAttr.height;
// 			
// 			
// 			
// 			// cycle last position
// 		}
// 	}
	
	for(evcnt = 0; XPending(xs->display) && evcnt < max_events; evcnt++) {
		XNextEvent(xs->display, &xev);
		KeySym sym;
		char c;
		
		// capture expose events cause they're useful. fullscreen games are for wimps who can't ultratask.
		if(xev.type == Expose) {
			// update some standard numbers
			// there's currently risk of a race condition if anything tries to read winAttr before the expose event fires
			XGetWindowAttributes(xs->display, xs->clientWin, &xs->winAttr);
			
			glViewport(0, 0, xs->winAttr.width, xs->winAttr.height);
			
			if(xs->onExpose)
				(*xs->onExpose)(xs, xs->onExposeData);
			
			
			
			xs->ready = 1;
		}
		
		if(xev.type == KeyPress) {
			double gt = GameTimeFromXTime(xev.xkey.time);
			
			st->keyState[xev.xkey.keycode] |= IS_KEYPRESSED | IS_KEYDOWN;
			
			int slen = XLookupString(&xev, &c, 1, &sym, NULL);
			
			iev.type = EVENT_KEYDOWN;
			iev.time = gt;
			iev.keysym = sym;
			iev.character = c;
			//.iev.keycode = xev.xkey.keycode;
			iev.kbmods = TranslateModState(xev.xkey.state);
			
			InputFocusStack_Dispatch(ifs, &iev);
		}
		if(xev.type == KeyRelease) {
			double gt = GameTimeFromXTime(xev.xkey.time);
			
			st->keyState[xev.xkey.keycode] &= !IS_KEYDOWN;
			
			int slen = XLookupString(&xev, &c, 1, &sym, NULL);
			
			iev.type = EVENT_KEYUP;
			iev.time = gt;
			iev.keysym = sym;
			iev.character = c;
			iev.kbmods = TranslateModState(xev.xkey.state);
			
			InputFocusStack_Dispatch(ifs, &iev);
			
			if(isprint(c)) {
				iev.type = EVENT_TEXT;
				InputFocusStack_Dispatch(ifs, &iev);
			}
		}
		
		// mouse events
		if(xev.type == ButtonPress) {
			pixelPos.x = CLAMP(0, xev.xbutton.x, xs->winAttr.width);
			pixelPos.y = CLAMP(0, xs->winAttr.height - xev.xbutton.y, xs->winAttr.height);
			
			normPos.x = (float)xev.xbutton.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xbutton.y / (float)xs->winAttr.height); // opengl is inverted to X
			
			double gt = GameTimeFromXTime(xev.xbutton.time);
			
			iev.type = EVENT_MOUSEDOWN;
			iev.time = gt;
			iev.button = xev.xbutton.button;
			iev.kbmods = TranslateModState(xev.xbutton.state);
			
			InputFocusStack_Dispatch(ifs, &iev);
			
			if(st->inDrag) {
				// what? shouldn't get here.
				fprintf(stderr, "got ButtonPress during a drag.\n");
			}
			else { // for determining when to start a drag
				//printf("lastpress\n");
				st->lastPressTime = gt;
				st->lastPressPosPixels = pixelPos;
				st->lastPressPosNorm = normPos;
			}
			
		}
		if(xev.type == ButtonRelease) {
			
			pixelPos.x = CLAMP(0, xev.xbutton.x, xs->winAttr.width);
			pixelPos.y = CLAMP(0, xs->winAttr.height - xev.xbutton.y, xs->winAttr.height);
			
			normPos.x = (float)xev.xbutton.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xbutton.y / (float)xs->winAttr.height); // opengl is inverted to X
			
			double gt = GameTimeFromXTime(xev.xbutton.time);
			
			iev.intPos = pixelPos;
			iev.normPos = normPos;
			
			iev.time = gt;
			iev.button = xev.xbutton.button;
			iev.kbmods = TranslateModState(xev.xbutton.state);

			if(st->inDrag) { //printf("release in drag\n");
				iev.type = EVENT_DRAGSTOP;
				
				iev.intDragStart = st->lastPressPosPixels;
				iev.normDragStart = st->lastPressPosNorm;
				
				InputFocusStack_Dispatch(ifs, &iev);
				
				st->inDrag = 0;
			}
			else { //printf("released not in drag\n");
				//printf("non-drag: %f , %f, %f\n", gt, st->lastClickTime, st->doubleClickTime);
				if(gt - st->lastClickTime > st->doubleClickTime ) { //printf("  click\n");
					iev.type = EVENT_CLICK; // BUG: pause and wait for doubleclick?
				}
				else { //printf("  doubleclick\n");
					iev.type = EVENT_DOUBLECLICK; // BUG: only issued after initial click
				}
				InputFocusStack_Dispatch(ifs, &iev);
				
				iev.type = EVENT_MOUSEUP;
				InputFocusStack_Dispatch(ifs, &iev);
			}
			
			st->lastClickTime = gt;
			st->lastPressTime = -1;
		}
		
		if(xev.type == MotionNotify) {
			
			pixelPos.x = CLAMP(0, xev.xmotion.x, xs->winAttr.width);
			pixelPos.y = CLAMP(0, xs->winAttr.height - xev.xmotion.y, xs->winAttr.height);
			
			normPos.x = (float)xev.xmotion.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xmotion.y / (float)xs->winAttr.height); // opengl is inverted to X
			invNormPos.x = (float)xev.xmotion.x / (float)xs->winAttr.width;
			invNormPos.y = (float)xev.xmotion.y / (float)xs->winAttr.height;
			
			double gt = GameTimeFromXTime(xev.xmotion.time);
			
			
			// check if the mouse actually moved
			if(st->lastCursorPosPixels.x == pixelPos.x && st->lastCursorPosPixels.y == pixelPos.y) {
				// not sure if X sends events without actual movment.
				// let's find out
				printf("mouse didn't move in XMotionNotify event.\n");
			}
			
			// check for drag start
			if(!st->inDrag && st->lastPressTime > 0) {
				float dist = vDist2i(&st->lastPressPosPixels, &pixelPos);
				if(dist > st->dragMinDist) {
					//printf("-motion drag start\n");
					st->inDrag = 1;
				}
			}
			
			// mouse move event
			if(st->inDrag) {
				iev.type = EVENT_DRAGMOVE;
			}
			else {
				iev.type = EVENT_MOUSEMOVE;
			}
			iev.intPos = pixelPos;
			iev.normPos = normPos;
			
			iev.time = gt;
			iev.button = -1;
			iev.kbmods = TranslateModState(xev.xmotion.state);
			
			InputFocusStack_Dispatch(ifs, &iev);
			
			
			
			st->lastCursorPos = normPos;
			st->lastCursorPosPixels = pixelPos;
			st->lastMoveTime = gt;
		}
		
		
		
	}
	
	
	
}


void XStuff_hideCursor(XStuff* xs) {
	XDefineCursor(xs->display, xs->clientWin, xs->noCursor);
}

void XStuff_showCursor(XStuff* xs) {
	XUndefineCursor(xs->display, xs->clientWin); 
}




void initGLEW() {
	GLenum err;
	
	// this is some global that GLEW declares elsewhere
	glewExperimental = GL_TRUE; // workaround for broken glew code in 1.10 and below (at least) - gl3.2+ contexts fail otherwise
	
	err = glewInit();
	if (GLEW_OK != err) {
		// we're fucked, just bail
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
		exit(1);
	}
	
	fprintf(stdout, "Initialized GLEW %s\n", glewGetString(GLEW_VERSION));
	glerr("existing error on glew init");
}
