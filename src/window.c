#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xfixes.h> // clipboard notification
#include <X11/cursorfont.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "utilities.h"
#include "c3dlas/c3dlas.h"
#include "window.h"
#include "clipboard.h"



// lag between the X server and the game's internal timekeeping, for adjusting events 
static double game_server_diff = 0;

void initGLEW();

 
 
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;


const int context_attr[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
	GLX_CONTEXT_MINOR_VERSION_ARB, 3,
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

#include <unistd.h>

int event_base, error_base;

// called when the user changes a clipboard buffer inside this app
static void clipNotify(int which, XStuff* xs) {

	Atom a;
	switch(which) {
		case CLIP_SELECTION: a = xs->primaryID; break;
		case CLIP_SECONDARY: a = xs->secondaryID; break;
		case CLIP_PRIMARY: a = xs->clipboardID; break;
		default: return;
	}
	
	// BUG:
	// For some unknown reason, calling this fn in rapid succession
	// (1000x per change when drag-selecting) causes the program,
	// or perhaps xlib within it, to gobble up a ton of memory which
	// it never frees. 
	XSetSelectionOwner(xs->display, a, xs->clientWin, CurrentTime);
//	printf("clipboard is mine\n");
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
	XColor black = {0};
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
	setWinAttr.event_mask = 
		  ExposureMask 
		| KeyPressMask 
		| KeyReleaseMask 
		| ButtonPressMask 
		| ButtonReleaseMask 
		| PointerMotionMask 
		| PropertyChangeMask
		| StructureNotifyMask
// 		| SelectionClear
// 		| SelectionRequestMask
// 		| SelectionMask
		;

	xs->clientWin = XCreateWindow(xs->display, xs->rootWin, 0, 0, 700, 700, 0, xs->vi->depth, InputOutput, xs->vi->visual, CWColormap | CWEventMask, &setWinAttr);

	XMapWindow(xs->display, xs->clientWin);
	
	XStoreName(xs->display, xs->clientWin, xs->windowTitle);
	
	// window manager interaction
	xs->wmProtocolsID = XInternAtom(xs->display, "WM_PROTOCOLS", False);
	xs->wmDeleteWindowID = XInternAtom(xs->display, "WM_DELETE_WINDOW", False);
  
	XSetWMProtocols(xs->display, xs->clientWin, &xs->wmDeleteWindowID, 1);
	
	// clipboard handling
	xs->clipboardID = XInternAtom(xs->display, "CLIPBOARD", False);
	xs->primaryID = XInternAtom(xs->display, "PRIMARY", False);
	xs->secondaryID = XInternAtom(xs->display, "SECONDARY", False);
	xs->selDataID = XInternAtom(xs->display, "XSEL_DATA", False);
	xs->utf8ID = XInternAtom(xs->display, "UTF8_STRING", False);
	xs->textID = XInternAtom(xs->display, "TEXT", False);
	xs->targetsID = XInternAtom(xs->display, "TARGETS", False);
	
	Clipboard_RegisterOnChange((void*)clipNotify, xs);
	
	XFixesQueryExtension(xs->display, &xs->XFixes_eventBase, &xs->XFixes_errorBase);
	XFixesSelectSelectionInput(xs->display, xs->clientWin, xs->clipboardID, XFixesSetSelectionOwnerNotifyMask);
	XFixesSelectSelectionInput(xs->display, xs->clientWin, xs->primaryID, XFixesSetSelectionOwnerNotifyMask);
	XFixesSelectSelectionInput(xs->display, xs->clientWin, xs->secondaryID, XFixesSetSelectionOwnerNotifyMask);

	//Bool XkbSetDetectableAutoRepeat (Display *display, Bool detectable, Bool *supported_rtrn); 
	
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
	
	
	glexit("");
	
	// disable vsync; it causes glXSwapBuffers to block on (at least) nVidia drivers
	// There are 3 different extensions used for this, by different drivers.
	do {
		PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
		glexit("");
		if(glXSwapIntervalEXT) {
			glXSwapIntervalEXT(xs->display, xs->clientWin, !!xs->gs->AppState_enableVSync); 
			break;
		}
		
		printf("glXSwapIntervalEXT not supported.\n");
		glexit("");
		
		
		PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
		glexit("");
		if(glXSwapIntervalSGI) {
			glXSwapIntervalSGI(!!xs->gs->AppState_enableVSync);
			break;
		} 
		
		printf("glXSwapIntervalSGI not supported.\n");
		glexit("");
	
	
		PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
		glexit("");
		if(glXSwapIntervalMESA) {
			glXSwapIntervalMESA(!!xs->gs->AppState_enableVSync);
			break; 
		}
		
		printf("glXSwapIntervalMESA not supported.\n");
		glexit("");
		
	} while(0);
	
	// have to have a current GLX context before initializing GLEW
	initGLEW();
	
#ifdef USE_KHR_DEBUG
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	
	glDebugMessageCallback((void*)_khr_debug_callback , NULL);
#endif
	
	
	// set up the empty cursor used to hide the cursor
	emptyPx = XCreateBitmapFromData(xs->display, xs->clientWin, zeros, 1, 1);
	xs->noCursor = XCreatePixmapCursor(xs->display, emptyPx, emptyPx, &black, &black, 0, 0);
	XFreePixmap(xs->display, emptyPx);
	
	xs->arrowCursor = XCreateFontCursor(xs->display, XC_left_ptr);
	xs->textCursor = XCreateFontCursor(xs->display, XC_xterm);
	xs->waitCursor = XCreateFontCursor(xs->display,  XC_watch);// XC_coffee_mug
	xs->hMoveCursor = XCreateFontCursor(xs->display,  XC_sb_h_double_arrow);// XC_coffee_mug
	xs->vMoveCursor = XCreateFontCursor(xs->display,  XC_sb_v_double_arrow);// XC_coffee_mug
	
	return 0;
}




#define CLAMP(min, mid, max) MIN(max, MAX(mid, min))



int processEvents(XStuff* xs, InputState* st, InputEvent* iev, int max_events) {
	
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
	
	iev->is = st;
	
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
			continue;
		}
		else if(xev.type == ClientMessage) { // wm events
			XClientMessageEvent* cev = (XClientMessageEvent*)&xev;
			if(cev->message_type == xs->wmProtocolsID) {
				
				// the X button in the window title bar
				if(cev->data.l[0] == xs->wmDeleteWindowID) {
					int denyExit = 0; 
					
					if(xs->onTryExit) denyExit = xs->onTryExit(xs, xs->onTryExitData);
					if(!denyExit) {
						glXDestroyContext(xs->display, xs->glctx);
						XDestroyWindow(xs->display, xs->clientWin);
						XCloseDisplay(xs->display);
						exit(0);
					}
				}
				
			}
		}
		else if(xev.type == ConfigureNotify) { // window resizes
			XConfigureEvent* conf = (XConfigureEvent*)&xev;
			
			if(conf->width != xs->winSize.x || conf->height != xs->winSize.y) {
				xs->winSize.x = conf->width;
				xs->winSize.y = conf->height;
				
				if(xs->onResize) xs->onResize(xs, xs->onResizeData);
			}
			
		}
		else if(xev.type == SelectionClear) {
//			printf("selection clear\n");
// 			xev.xselectionclear
			continue;
		}
		else if(xev.type == xs->XFixes_eventBase + XFixesSelectionNotify) {
			// ignore our own actions
			if(((XFixesSelectionNotifyEvent*)&xev)->owner == xs->clientWin) continue;
//			printf("xfixes selection notify\n");
			
			Atom buf = ((XFixesSelectionNotifyEvent*)&xev)->selection;
			XConvertSelection(xs->display, buf, xs->utf8ID, xs->selDataID, xs->clientWin, CurrentTime);
			
			continue;
		}
		else if(xev.type == SelectionNotify) {
// 			xev.xselection
// 			.property
//			printf("regular selection notify\n");
			Atom actualType;
			unsigned int resultBitsPerItem;
			unsigned long resultItemCount;
			unsigned long remainingBytes;
			unsigned char* result;
			 /*
			XGetWindowProperty(
				Display *display, Window w, 
				Atom property, 
				long long_offset, 
				long long_length, 
				Bool delete, 
				Atom req_type, 
				Atom *actual_type_return, 
				int *actual_format_return, 
				unsigned long *nitems_return, 
				unsigned long *bytes_after_return, 
				unsigned char **prop_return
			);*/ 
			
// 			 printf("selnotify\n");
			
			XGetWindowProperty(xs->display, xs->clientWin, 
				xev.xselection.property, //xs->selDataID, // property id
				0, LONG_MAX/4,  // offset/max
				False, // delete 
				AnyPropertyType, // requested type
				&actualType, 
				&resultBitsPerItem, 
				&resultItemCount, 
				&remainingBytes, 
				&result
			);
			
			// yes, this seems backwards but it's correct
			int which;
			if(xev.xselection.selection == xs->primaryID) which = CLIP_SELECTION;
			else if(xev.xselection.selection == xs->secondaryID) which = CLIP_SECONDARY;
			else if(xev.xselection.selection == xs->clipboardID) which = CLIP_PRIMARY;
			else {
 				printf("invalid selection buffer (clipboard) from X\n");
				continue;
			}
			
			Clipboard_SetFromOS(which, result, (resultBitsPerItem / 8) * resultItemCount, 1);
			
// 			printf("result %d '%.*s'\n", resultBitsPerItem, (int)resultItemCount, result);
			
			XFree(result);
			continue;
 		
		}
		else if(xev.type == SelectionRequest) {
// 			xev.xselectionrequest.selection
//			continue;
//			printf("selection request ");
			
			if(((XSelectionRequestEvent*)&xev)->requestor == xs->clientWin) {
//				printf("ignored\n");
				continue;
			}
			
//			printf("RUN requestor: %ld, me: %ld\n",
//				((XSelectionRequestEvent*)&xev)->requestor,
//				xs->clientWin 
//			);

			int which;
			if(xev.xselectionrequest.selection == xs->clipboardID) which = CLIP_PRIMARY;
			else if(xev.xselectionrequest.selection == xs->secondaryID) which = CLIP_SECONDARY;
			else if(xev.xselectionrequest.selection == xs->primaryID) which = CLIP_SELECTION;
			else continue;
			
			char* txt;
			size_t len;
			Clipboard_GetFromOS(which, &txt, &len, NULL);
// 			printf("sel req 2 '%.*s'\n", len, txt);
			
			
			if(xev.xselectionrequest.target == xs->targetsID) {
// 				printf("sending target list\n");
				Atom alist[3];
				alist[0] = xs->targetsID;
				alist[1] = xs->textID;
				alist[2] = xs->utf8ID;
				XChangeProperty(
					xs->display, 
					xev.xselectionrequest.requestor,
					xev.xselectionrequest.property, 
					xev.xselectionrequest.target,
					32, 
					PropModeReplace,
					(unsigned char*)alist, 
					3
				);
			}
			else {
// 				printf("sending regular data\n");
				XChangeProperty(
					xs->display,
					xev.xselectionrequest.requestor, 
					xev.xselectionrequest.property,
	// 				xev.xselectionrequest.target,
					xs->utf8ID,
					8,
					PropModeReplace,
					txt,
					len
				);
			}
			
// 			printf("atom name: %s\n", XGetAtomName(xs->display, xev.xselectionrequest.target));
// 			printf("prop name: %s\n", XGetAtomName(xs->display, xev.xselectionrequest.property));
// 			printf("target name: %s\n", XGetAtomName(xs->display, xev.xselectionrequest.target));
// 			printf("selectoin name: %s\n", XGetAtomName(xs->display, xev.xselectionrequest.selection));
			XSelectionEvent selevt = {
				.type = SelectionNotify,
				.display = xs->display,
				.requestor = xev.xselectionrequest.requestor,
				.selection = xev.xselectionrequest.selection,
				.time = xev.xselectionrequest.time,
				.target = xev.xselectionrequest.target,
				.property = xev.xselectionrequest.property,
			};
			
			XSendEvent(xs->display, selevt.requestor, True, NoEventMask, (XEvent*)&selevt);
			
			continue;
		}
		if(xev.type == KeyPress) {
			double gt = GameTimeFromXTime(xev.xkey.time);
			
			st->keyState[xev.xkey.keycode] |= IS_KEYPRESSED | IS_KEYDOWN;
			
			int slen = XLookupString(&xev.xkey, &c, 1, &sym, NULL);
			
			iev->type = EVENT_KEYDOWN;
			iev->time = gt;
			iev->keysym = sym;
			iev->character = c;
			//.iev->keycode = xev.xkey.keycode;
			iev->kbmods = TranslateModState(xev.xkey.state);
			
// 			InputFocusStack_Dispatch(ifs, &iev);
			return 1;
		}
		if(xev.type == KeyRelease) {
			double gt = GameTimeFromXTime(xev.xkey.time);
			
			st->keyState[xev.xkey.keycode] &= !IS_KEYDOWN;
// 			int keysym = XKeycodeToKeysym(xs->display, xev.xkey.keycode, 0);
// 			printf("sym: %d '%c'\n", keysym, keysym);
			
			int slen = XLookupString(&xev.xkey, &c, 1, &sym, NULL);
			
			iev->type = EVENT_KEYUP;
			iev->time = gt;
			iev->keysym = sym;
			iev->character = c;
			iev->kbmods = TranslateModState(xev.xkey.state);
			
// 			InputFocusStack_Dispatch(ifs, &iev);
			return 1;
		}
		
		// mouse events
		if(xev.type == ButtonPress) {
			pixelPos.x = CLAMP(0, xev.xbutton.x, xs->winAttr.width);
			pixelPos.y = CLAMP(0, xev.xbutton.y, xs->winAttr.height);
			
			normPos.x = (float)xev.xbutton.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xbutton.y / (float)xs->winAttr.height); // opengl is inverted to X
			
			double gt = GameTimeFromXTime(xev.xbutton.time);
			
			iev->type = EVENT_MOUSEDOWN;
			iev->time = gt;
			iev->button = xev.xbutton.button;
			iev->kbmods = TranslateModState(xev.xbutton.state);
			
// 			InputFocusStack_Dispatch(ifs, &iev);
			
			//printf("lastpress\n");
			st->lastPressTime = gt;
			st->lastPressPosPixels = pixelPos;
			st->lastPressPosNorm = normPos;
			return 1;
		}
		if(xev.type == ButtonRelease) {
			
			pixelPos.x = CLAMP(0, xev.xbutton.x, xs->winAttr.width);
			pixelPos.y = CLAMP(0, xev.xbutton.y, xs->winAttr.height);
			
			normPos.x = (float)xev.xbutton.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xbutton.y / (float)xs->winAttr.height); // opengl is inverted to X
			
			double gt = GameTimeFromXTime(xev.xbutton.time);
			
			iev->intPos = pixelPos;
			iev->normPos = normPos;
			
			iev->time = gt;
			iev->button = xev.xbutton.button;
			iev->kbmods = TranslateModState(xev.xbutton.state);
			
			iev->type = EVENT_MOUSEUP;
			
			st->lastClickTime = gt;
			st->lastPressTime = -1;
			
			return 1;
		}
		
		if(xev.type == MotionNotify) {
			
			pixelPos.x = xev.xmotion.x;
			pixelPos.y = xev.xmotion.y;
			
			normPos.x = (float)xev.xmotion.x / (float)xs->winAttr.width;
			normPos.y = 1.0 - ((float)xev.xmotion.y / (float)xs->winAttr.height); // opengl is inverted to X
			invNormPos.x = (float)xev.xmotion.x / (float)xs->winAttr.width;
			invNormPos.y = (float)xev.xmotion.y / (float)xs->winAttr.height;
			
			double gt = GameTimeFromXTime(xev.xmotion.time);
			
			
			// check if the mouse actually moved
			if(st->lastCursorPosPixels.x == pixelPos.x && st->lastCursorPosPixels.y == pixelPos.y) {
				// not sure why exactly X sends these events, but it does happen when the mouse
				// is dragging outside the client window.
				
// 				printf("mouse didn't move in XMotionNotify event.\n");
// 				continue;
			}
			
			iev->type = EVENT_MOUSEMOVE;
			iev->intPos = pixelPos;
			iev->normPos = normPos;
			
			iev->time = gt;
			iev->button = -1;
			iev->kbmods = TranslateModState(xev.xmotion.state);
			
// 			InputFocusStack_Dispatch(ifs, &iev);
			
			st->lastCursorPos = normPos;
			st->lastCursorPosPixels = pixelPos;
			st->lastMoveTime = gt;
			
			return 1;
		}
		
	}
	
	// out of events
	return 0;
}


void XStuff_hideCursor(XStuff* xs) {
	XDefineCursor(xs->display, xs->clientWin, xs->noCursor);
}

void XStuff_showCursor(XStuff* xs) {
	XUndefineCursor(xs->display, xs->clientWin); 
}



void XStuff_SetWindowTitle(XStuff* xs, char* title) {
	XStoreName(xs->display, xs->clientWin, title);
}

void XStuff_SetMouseCursor(XStuff* xs, int index) {
	static int x = 0;
	
	if(index <= 0) {
		XUndefineCursor(xs->display, xs->clientWin); 
		return;
	}
	
	
	Cursor c;
	switch(index) {
		case 1:
		default:
			c = xs->arrowCursor;
			break;
		case 2: c = xs->textCursor; break;
		case 3: c = xs->waitCursor; break;
		case 4: c = xs->hMoveCursor; break;
		case 5: c = xs->vMoveCursor; break;
	}
	
	XDefineCursor(xs->display, xs->clientWin, c);
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
