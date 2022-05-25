#ifndef __gputk_gui_H__
#define __gputk_gui_H__

#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>

#include "../gui_deps.h"

#include "gui_settings.h"
#include "commands.h"


struct GUIManager;
typedef struct GUIManager GUIManager;





struct ShaderColor4 {
	uint8_t r,g,b,a;
} __attribute__ ((packed));

struct ShaderColor3 {
	uint8_t r,g,b;
} __attribute__ ((packed));

struct ShaderBox {
	float l, t, r, b;
} __attribute__ ((packed));


/*
For triangles (type = 6)
	pos.tl = centroid
	pos.b = height
	pos.r = base
	
*/

typedef struct GUIUnifiedVertex {
	struct ShaderBox pos;
	struct ShaderBox clip;
	uint8_t texIndex1, texIndex2, texFade, guiType; 
	struct { uint16_t x, y; } texOffset1, texOffset2;
	struct { uint16_t x, y; } texSize1, texSize2;
	
	struct ShaderColor4 fg;
	struct ShaderColor4 bg;
	
	float z, alpha, rot, opt2;
	
} __attribute__ ((packed)) GUIUnifiedVertex;


#define GUI_COLOR4_TO_SHADER(c4) ((struct ShaderColor4){(c4).r * 255, (c4).g * 255, (c4).b * 255, (c4).a * 255})
#define GUI_COLOR3_TO_SHADER(c3) ((struct ShaderColor3){(c4).r * 255, (c4).g * 255, (c4).b * 255})
#define COLOR4_FROM_HEX(r,g,b,a) ((struct Color4){(float)r / 255.0, (float)g / 255.0, (float)b / 255.0, (float)a / 255.0});

#define GUI_AABB2_TO_SHADER(c) ((struct ShaderBox){.l = (c).min.x, .t = (c).min.y, .r = (c).max.x, .b = (c).max.y})

struct GUIManager;

struct GUIRenderParams;
typedef struct GUIRenderParams GUIRenderParams;



struct GUIEvent;
typedef struct GUIEvent GUIEvent;


// bubbling: 0=none, just the target
//           1=rise until cancelled
//           2=all parents

//  X(name, bubbling) 
#define GUIEEVENTTYPE_LIST \
	X(Any, 0) \
	X(Click, 1) \
	X(DoubleClick, 1) \
	X(MiddleClick, 1) \
	X(RightClick, 1) \
	X(AuxClick, 1) \
	X(MouseDown, 2) \
	X(MouseUp, 2) \
	X(MouseMove, 2) \
	X(KeyDown, 1) \
	X(KeyUp, 1) \
	X(MouseEnter, 1) \
	X(MouseLeave, 1) \
	X(MouseEnterChild, 2) \
	X(MouseLeaveChild, 2) \
	\
	X(DragStart, 1) \
	X(DragStop, 1) \
	X(DragMove, 1) \
	\
	X(Scroll, 1) \
	X(ScrollDown, 1) \
	X(ScrollUp, 1) \
	\
	X(Paste, 1) \
	\
	X(User, 1) \
	X(UserAll, 1)


enum GUIEventType {
#define X(name, b) GUIEVENT_##name,
	GUIEEVENTTYPE_LIST
#undef X
};
enum GUIEventType_Bit {
#define X(name, b) GUIEVENT_##name##_BIT = (1 << GUIEVENT_##name),
	GUIEEVENTTYPE_LIST
#undef X
};

static char GUIEventBubbleBehavior[] = {
	#define X(name, bubble) [GUIEVENT_##name] = bubble,
		GUIEEVENTTYPE_LIST
	#undef X
};


// mouse buttons: 
// 1 - left
// 2 - mid
// 3 - right
// 4 - scroll up
// 5 - scroll down
// 6 - scroll left
// 7 - scroll right
// 8 - front left side (on my mouse)
// 9 - rear left side (on my mouse)

// specific keys
#define GUIMODKEY_LSHIFT (1 << 1)
#define GUIMODKEY_RSHIFT (1 << 2)
#define GUIMODKEY_LCTRL  (1 << 3)
#define GUIMODKEY_RCTRL  (1 << 4)
#define GUIMODKEY_LALT   (1 << 5)
#define GUIMODKEY_RALT   (1 << 6)
#define GUIMODKEY_LTUX   (1 << 7)
#define GUIMODKEY_RTUX   (1 << 8)
#define GUIMODKEY_MENU   (1 << 9)

// set if either L or R is pressed
#define GUIMODKEY_SHIFT  (1 << 30)
#define GUIMODKEY_CTRL   (1 << 29)
#define GUIMODKEY_ALT    (1 << 28)
#define GUIMODKEY_TUX    (1 << 27)


typedef struct GUIEvent {
	enum GUIEventType type;
	char* userType;
	
	double eventTime;

	union {
		Vector2 pos; // for mouse events; absolute position
		Vector2 size; // for window size
		void* userData; // for User events
	};
	
	union {
		int character; // for kb events
		int button; // for mouse events
	};
	union {
		Vector2 dragStartPos;
		int keycode;
		ptrdiff_t userSize; // for User events
	};
	
	unsigned int modifiers;
	
	char multiClick;
	char cancelled;
	char requestRedraw;
} GUIEvent;




#define GUIMOUSECURSOR_DYNAMIC   -1
#define GUIMOUSECURSOR_ARROW   0x01
#define GUIMOUSECURSOR_TEXT    0x02
#define GUIMOUSECURSOR_WAIT    0x03
#define GUIMOUSECURSOR_H_MOVE  0x04
#define GUIMOUSECURSOR_V_MOVE  0x05

// do not z-sort the rendered elements of this window
#define GUI_NO_SORT      0x0001ul

// do not clip this window to its parent's bounds (popups, modals, menus)
#define GUI_NO_CLIP      0x0002ul


// just a way of grouping the memory data with the buffer. Nothing more.
typedef struct GUIString {
	size_t alloc;
	size_t len;
	char* data;
} GUIString;

typedef int (*GUIEditFilterFn)(GUIString*, GUIEvent*, int /*pos*/, void* /*user_data*/); 


typedef struct GUIElementData {
	void* id;
	int age; // in frames
	void* data;
	void (*freeFn)(void*);
} GUIElementData;

typedef struct GUIWindow {
	void* id;
	Vector2 size; // declared size
	AABB2 clip; // relative to the parent
	AABB2 absClip; // calculated absolute coordinates used for mouse and rendering
	float z;
	unsigned long flags;

	struct GUIWindow* parent;
	VEC(struct GUIWindow*) children;
	
	int vertCount;
	int vertAlloc;
	GUIUnifiedVertex* vertBuffer;
	
} GUIWindow;


#include "opts_structs.h"


/*

*/
typedef struct GUIManager {
	int maxInstances; // limited by vmem ring buffer settings
	PCBuffer instVB;
	GLuint vao;
	
	int totalVerts; // per-frame running count used to allocate enough storage for the full flattened tree
	int vertCount; // head position used while flattening the tree
	int vertAlloc;
	GUIUnifiedVertex* vertBuffer;
	
	Vector2i screenSize;
	Vector2 screenSizef;
	
	float fontClipLow, fontClipHigh;
	float fontClipGap;
	
	
	FontManager* fm;
	TextureAtlas* ta;
	VEC(GUI_Cmd) cmdList;
	VEC(GUI_CmdElementInfo) cmdElements;
	HT(int) cmdElementLookup;
	VEC(GUI_CmdModeInfo) commandModes;
	GUI_Cmd tmpCmd;
	
	
	// immediate mode stuff
	char drawMode;// only set to 1 when primitives should be written to the draw buffers
	void* renderRootData;
	void (*renderRootFn)(void*, GUIManager*, Vector2 /*tl*/, Vector2 /*sz*/, PassFrameParams*);
	
	VEC(AABB2) clipStack;
	AABB2 curClip;
	float curZ;
	float fontSize;

	GUIEvent curEvent;
	float scrollDist;
	char mouseWentUp[16];
	char mouseWentDown[16];
	char mouseIsDown[16];
	char mouseIsDragging[16];
	Vector2 mouseDragStartPos[16]; // absolute coordinates
	void* mouseWinID;
	
	void* hotID;
	void* activeID;
	
	void* hotData;
	void* activeData;
	
	void (*hotFree)(void*);
	void (*activeFree)(void*);
	
	VEC(GUIElementData) elementData;
	
	// per-frame event queue
	VEC(GUIEvent) events;
	
	
	double time;
	double timeElapsed;
	
	struct {
		long alloc;
		long cnt;
		GUIWindow* buf;
	} windowHeap;
	VEC(GUIWindow*) windowStack;
	GUIWindow* curWin;
	GUIWindow* rootWin;
	
	// ---------------
	
	void (*windowTitleSetFn)(void*, char*);
	void* windowTitleSetData;
	
	void (*mouseCursorSetFn)(void*, int);
	void* mouseCursorSetData;
	
	// input 
	Vector2 lastMousePos;
	char mouseIsOutOfWindow;
	
	char isDragging;
	char isMouseDown;
	int dragButton;
	Vector2 dragStartPos;
	float dragStartTime;
	float minDragDist;
	
	struct {
		float time;
		Vector2 pos;
	} clickHistory[16][3]; // [button][entry]
	float doubleClickTime;
	float tripleClickTime;
	float quadClickTime;
	float multiClickDist;
	
	int defaultCursor;
	int currentCursor;
	
	char useSoftCursor;
	char* softCursorName;
	Vector2 softCursorSize;
	
	GUISettings defaults;
	
	// temp 
	GLuint fontAtlasID;
	GLuint atlasID;
	
	VEC(GLuint64) texHandles;
	
	GUISettings* gs;
	
} GUIManager;



void GUIManager_Init(GUIManager* gm, GUISettings* gs);
void GUIManager_InitGL(GUIManager* gm);
GUIManager* GUIManager_alloc();

GUIWindow* GUIWindow_new(GUIManager* gm, GUIWindow* parent);
void GUIManager_RunRenderPass(GUIManager* gm, PassFrameParams* pfp, int isDraw);

RenderPass* GUIManager_CreateRenderPass(GUIManager* gm);
PassDrawable* GUIManager_CreateDrawable(GUIManager* gm);

void GUIManager_SetMainWindowTitle(GUIManager* gm, char* title);
void GUIManager_SetCursor(GUIManager* gm, int cursor);



GUIFont* GUI_FindFont(GUIManager* gm, char* name);





void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp);
void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp);
void GUIManager_HandleKeyInput(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp);


// debugging
void gui_debugFileDumpVertexBuffer(GUIManager* gm, char* filePrefix, int fileNum);







#endif // __gputk_gui_h__
