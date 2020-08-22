#ifndef __EACSMB_GUI_H__
#define __EACSMB_GUI_H__

#include <stdatomic.h>

#include "common_gl.h"
#include "common_math.h"

#include "utilities.h"

#include "input.h"
#include "texture.h"
#include "pass.h"
#include "pcBuffer.h"

#include "font.h"


struct GameState;
typedef struct GameState GameState;

struct GUIManager;
typedef struct GUIManager GUIManager;



#define GUI_GRAV_TOP_LEFT      0x00
#define GUI_GRAV_LEFT_TOP      0x00
#define GUI_GRAV_CENTER_LEFT   0x01
#define GUI_GRAV_LEFT_CENTER   0x01
#define GUI_GRAV_BOTTOM_LEFT   0x02
#define GUI_GRAV_LEFT_BOTTOM   0x02
#define GUI_GRAV_CENTER_BOTTOM 0x03
#define GUI_GRAV_BOTTOM_CENTER 0x03
#define GUI_GRAV_BOTTOM_RIGHT  0x04
#define GUI_GRAV_RIGHT_BOTTOM  0x04
#define GUI_GRAV_CENTER_RIGHT  0x05
#define GUI_GRAV_RIGHT_CENTER  0x05
#define GUI_GRAV_TOP_RIGHT     0x06
#define GUI_GRAV_RIGHT_TOP     0x06
#define GUI_GRAV_CENTER_TOP    0x07
#define GUI_GRAV_TOP_CENTER    0x07
#define GUI_GRAV_CENTER        0x08
#define GUI_GRAV_CENTER_CENTER 0x08


typedef struct Color4 {
	float r,g,b,a;
} Color4;

typedef struct Color3 {
	float r,g,b;
} Color3;


struct ShaderColor4 {
	uint8_t r,g,b,a;
} __attribute__ ((packed));

struct ShaderColor3 {
	uint8_t r,g,b;
} __attribute__ ((packed));

struct ShaderBox {
	float l, t, r, b;
} __attribute__ ((packed));

typedef struct GUIUnifiedVertex {
	struct ShaderBox pos;
	struct ShaderBox clip;
	uint8_t texIndex1, texIndex2, texFade, guiType; 
	struct { uint16_t x, y; } texOffset1, texOffset2;
	struct { uint16_t x, y; } texSize1, texSize2;
	
	struct ShaderColor4 fg;
	struct ShaderColor4 bg;
	
	float z, alpha, opt1, opt2;
	
} __attribute__ ((packed)) GUIUnifiedVertex;


#define GUI_COLOR4_TO_SHADER(c4) ((struct ShaderColor4){(c4).r * 255, (c4).g * 255, (c4).b * 255, (c4).a * 255})
#define GUI_COLOR3_TO_SHADER(c3) ((struct ShaderColor3){(c4).r * 255, (c4).g * 255, (c4).b * 255})
#define COLOR4_FROM_HEX(r,g,b,a) ((struct Color4){(float)r / 255.0, (float)g / 255.0, (float)b / 255.0, (float)a / 255.0});

#define GUI_AABB2_TO_SHADER(c) ((struct ShaderBox){.l = (c).min.x, .t = (c).min.y, .r = (c).max.x, .b = (c).max.y})


typedef union GUIObject GUIObject;
struct GUIManager;

struct GUIRenderParams;
typedef struct GUIRenderParams GUIRenderParams;


struct gui_vtbl {
	void (*UpdatePos)(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp);
	void (*Render)(GUIObject* go, PassFrameParams* pfp);
	void (*Delete)(GUIObject* go);
	void (*Reap)(GUIObject* go);
	void (*Resize)(GUIObject* go, Vector2 newSz); // exterior size
	Vector2 (*GetClientSize)(GUIObject* go);
	void    (*SetClientSize)(GUIObject* go, Vector2 cSize); // and force exterior size to match
	Vector2 (*RecalcClientSize)(GUIObject* go); // recalc client size from the client children and call SetClientSize
	GUIObject* (*HitTest)(GUIObject* go, Vector2 testPos);
	
	void (*AddClient)(GUIObject* parent, GUIObject* child);
	void (*RemoveClient)(GUIObject* parent, GUIObject* child);
	Vector2 (*SetScrollPct)(GUIObject* go, Vector2 pct); // returns the final value
	Vector2 (*SetScrollAbs)(GUIObject* go, Vector2 absPos);
};


struct GUIEvent;
typedef struct GUIEvent GUIEvent;
typedef void (*GUI_EventHandlerFn)(GUIObject*, GUIEvent*);

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
	X(GainedFocus, 0) \
	X(LostFocus, 0) \
	X(ParentResize, 0) \
	X(Paste, 1) 


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

struct GUIEventHandler_vtbl {
#define X(name, b) GUI_EventHandlerFn name;
	GUIEEVENTTYPE_LIST
#undef X
};

static char GUIEventBubbleBehavior[] = {
	#define X(name, bubble) [GUIEVENT_##name] = bubble,
		GUIEEVENTTYPE_LIST
	#undef X
};

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
	
	double eventTime;
	Vector2 eventPos;
	GUIObject* originalTarget;
	GUIObject* currentTarget;
	
	union {
		Vector2 pos; // for mouse events; absolute position
		Vector2 size; // for window size
	};
	
	union {
		int character; // for kb events
		int button; // for mouse events
	};
	union {
		Vector2 dragStartPos;
		int keycode;
	};
	
	unsigned int modifiers;
	
	char multiClick;
	char cancelled;
	char requestRedraw;
} GUIEvent;


typedef int  (*GUI_OnClickFn)(GUIObject* go, Vector2 clickPos);
typedef void (*GUI_OnMouseEnterFn)(GUIEvent* e);
typedef void (*GUI_OnMouseLeaveFn)(GUIEvent* e);


#define GUIMOUSECURSOR_ARROW 0x01
#define GUIMOUSECURSOR_TEXT  0x02
#define GUIMOUSECURSOR_WAIT  0x03

#define GUI_MAXIMIZE_X 0x0001
#define GUI_MAXIMIZE_Y 0x0002
#define GUI_NOCLIP     0x0004

typedef struct GUIHeader {
	struct GUIManager* gm;
	GUIObject* parent;
	struct gui_vtbl* vt;
	struct GUIEventHandler_vtbl* event_vt;
	
	VEC(struct {
		enum GUIEventType type;
		GUI_EventHandlerFn cb;
	}) dynamicHandlers;
	
	char* name; // used by config loader atm

	// fallback for easy hit testing
	VEC(union GUIObject*) children;
	
	Vector2 topleft; // relative to parent (and window padding)
	Vector2 size; // absolute
	float scale;
	float alpha;
	float z; // relative to the parent
	
	// calculated absolute coordinates of the top left corner
	// updated every frame before any rendering or hit testing
	Vector2 absTopLeft; 
	// calculated tl coords relative to the parent, factoring in gravity and GRP parent offset
	Vector2 relTopLeft; 
	// calculated absolute clipping box. this element may be entirely clipped.
	AABB2 absClip;
	// calculated absolute z index
	float absZ;
	
	
	unsigned int flags   : 16;
	unsigned int gravity :  8;
	unsigned int hidden  :  1;
	unsigned int deleted :  1;
	
	int cursor;
	
} GUIHeader;



// Animations
#include "ui/animations/pulse.h"



// GUI elements
#include "ui/window.h"
#include "ui/text.h"
#include "ui/textf.h"
#include "ui/button.h"
#include "ui/scrollWindow.h"
#include "ui/simpleWindow.h"
#include "ui/image.h"
#include "ui/imgButton.h"
#include "ui/tree.h"
#include "ui/edit.h"
#include "ui/selectBox.h"
#include "ui/slider.h"
#include "ui/columnLayout.h"
#include "ui/gridLayout.h"
#include "ui/tabBar.h"
#include "ui/tabControl.h"
#include "ui/formControl.h"
#include "ui/monitors.h"
#include "ui/debugAdjuster.h"
#include "ui/structAdjuster.h"
#include "ui/performanceGraph.h"



union GUIObject {
	GUIHeader h; // legacy
	GUIHeader header;
	GUIText text;
	GUIButton button;
	GUIWindow window;
	GUISimpleWindow simpleWindow;
	GUIImage image;
	GUIColumnLayout columnLayout;
	GUIGridLayout gridLayout;
	GUIValueMonitor valueMonitor;
	GUIDebugAdjuster debugAdjuster;
	GUISlider Slider;
	GUITabControl TabControl;
// 	GUIScrollbar Scrollbar;
	GUIEdit Edit;
	GUIStructAdjuster structAdjuster;
	GUIImageButton ImageButton;
	GUIPerformanceGraph PerformanceGraph;
};




/*
The general idea is this:
The gui is held in a big tree. The tree is walked depth-first from the bottom up, resulting in
a mostly-sorted list. A sort is then run on z-index to ensure proper order. The list is then
rendered all at once through a single unified megashader using the geometry stage to expand
points into quads.

*/
typedef struct GUIManager {
	int maxInstances;
	PCBuffer instVB;
	GLuint vao;
	
	int elementCount;
	int elementAlloc;
	GUIUnifiedVertex* elemBuffer;
	
	Vector2i screenSize;
	
	GUIObject* root;
	VEC(GUIHeader*) reapQueue; 
	
	FontManager* fm;
	TextureAtlas* ta;
	
	void (*windowTitleSetFn)(void*, char*);
	void* windowTitleSetData;
	
	void (*mouseCursorSetFn)(void*, int);
	void* mouseCursorSetData;
	
	// input 
	Vector2 lastMousePos;
	GUIHeader* lastHoveredObject;
	char mouseIsOutOfWindow;
	
	char isDragging;
	char isMouseDown;
	int dragButton;
	Vector2 dragStartPos;
	float dragStartTime;
	GUIHeader* dragStartTarget;
	float minDragDist;
	
	struct {
		float time;
		int button;
	} clickHistory[3];
	float doubleClickTime;
	float tripleClickTime;
	
	int defaultCursor;
	int currentCursor;
	
	VEC(GUIObject*) focusStack;
	
	struct {
		GUIFont* font;
		float fontSize;
		struct Color4 textColor;
		struct Color4 editBgColor;
		struct Color4 editBorderColor;
		float         editHeight;
		float         editWidth;
		struct Color4 buttonTextColor;
		struct Color4 buttonHoverTextColor;
		struct Color4 buttonDisTextColor;
		struct Color4 buttonBgColor;
		struct Color4 buttonHoverBgColor;
		struct Color4 buttonDisBgColor;
		struct Color4 buttonBorderColor;
		struct Color4 buttonHoverBorderColor;
		struct Color4 buttonDisBorderColor;
		struct Color4 cursorColor;
		struct Color4 tabTextColor;
		struct Color4 tabBorderColor;
		struct Color4 tabActiveBgColor;
		struct Color4 tabHoverBgColor;
		struct Color4 tabBgColor;
		struct Color4 windowBgBorderColor;
		float         windowBgBorderWidth;
		struct Color4 windowBgColor;
		struct Color4 windowTitleBorderColor;
		float         windowTitleBorderWidth;
		struct Color4 windowTitleColor;
		struct Color4 windowTitleTextColor;
		struct Color4 windowCloseBtnBorderColor;
		float         windowCloseBtnBorderWidth;
		struct Color4 windowCloseBtnColor;
		struct Color4 windowScrollbarColor;
		struct Color4 windowScrollbarBorderColor;
		float         windowScrollbarBorderWidth;
		struct Color4 selectBgColor;
		struct Color4 selectBorderColor;
		struct Color4 selectTextColor;
		// TODO: font name, size 
	} defaults;
	
	
	// temp 
	GLuint fontAtlasID;
	GLuint atlasID;
	
	VEC(GLuint64) texHandles;
	
	GlobalSettings* gs;
	
} GUIManager;



void GUIManager_init(GUIManager* gm, GlobalSettings* gs);
void GUIManager_initGL(GUIManager* gm, GlobalSettings* gs);
GUIManager* GUIManager_alloc(GlobalSettings* gs);


RenderPass* GUIManager_CreateRenderPass(GUIManager* gm);
PassDrawable* GUIManager_CreateDrawable(GUIManager* gm);

void GUIManager_SetMainWindowTitle(GUIManager* gm, char* title);
void GUIManager_SetCursor(GUIManager* gm, int cursor);



void GUIManager_updatePos(GUIManager* gm, PassFrameParams* pfp);
void GUIManager_Reap(GUIManager* gm);


GUIObject* GUIObject_hitTest(GUIObject* go, Vector2 testPos);
GUIObject* GUIManager_hitTest(GUIManager* gm, Vector2 testPos);
// 
// void GUIObject_triggerClick(GUIObject* go, GUIEvent* e); 
void GUIObject_triggerClick(GUIObject* go, Vector2 testPos);
GUIObject* GUIManager_triggerClick(GUIManager* gm, Vector2 testPos);

GUIObject* GUIObject_findChild(GUIObject* obj, char* childName);

// focus stack
GUIObject* GUIManager_getFocusedObject(GUIManager* gm);
#define GUIManager_pushFocusedObject(gm, o) GUIManager_pushFocusedObject_(gm, &(o)->header)
void GUIManager_pushFocusedObject_(GUIManager* gm, GUIHeader* h);
GUIObject* GUIManager_popFocusedObject(GUIManager* gm);


void GUIResize(GUIHeader* gh, Vector2 newSz);


void guiTriggerClick(GUIEvent* e); 



#define GUIAddClient(p, o) GUIAddClient_((p) ? (&((GUIObject*)(p))->header) : NULL, &(o)->header)
void GUIAddClient_(GUIHeader* parent, GUIHeader* o);

#define GUIRemoveClient(p, o) GUIRemoveClient_((p) ? (&((GUIObject*)(p))->header) : NULL, &(o)->header)
void GUIRemoveClient_(GUIHeader* parent, GUIHeader* o);

#define GUIRegisterObject(p, o) GUIRegisterObject_((p) ? (&((GUIObject*)(p))->header) : NULL, &(o)->header)
void GUIRegisterObject_(GUIHeader* parent, GUIHeader* o);

#define GUIUnregisterObject(o) GUIUnregisterObject_(&(o)->header)
void GUIUnregisterObject_(GUIHeader* o);

// Delete marks things to be reaped later. It removes objects from the root tree.
#define GUIObject_Delete(o) GUIObject_Delete_(&(o)->header)
void GUIObject_Delete_(GUIHeader* h);

// Reap is for garbage collection. 
// It happens at a separate phase and does not depend on object relations. 
#define GUIObject_Reap(o) GUIObject_Reap_(&(o)->header)
void GUIObject_Reap_(GUIHeader* h);

#define GUIObject_AddClient(p, c) GUIObject_AddClient_(&(p)->header, &(c)->header)
void GUIObject_AddClient_(GUIHeader* parent, GUIHeader* client);

#define GUIObject_RemoveClient(p, c) GUIObject_RemoveClient_(&(p)->header, &(c)->header)
void GUIObject_RemoveClient_(GUIHeader* parent, GUIHeader* client);

#define GUIObject_SetScrollPct(o, pct) GUIObject_SetScrollPct_(&(o)->header, pct)
Vector2 GUIObject_SetScrollPct_(GUIHeader* go, Vector2 pct);

#define GUIObject_SetScrollAbs(o, abs) GUIObject_SetScrollAbs_(&(o)->header, abs)
Vector2 GUIObject_SetScrollAbs_(GUIHeader* go, Vector2 absPos);



void GUIManager_TriggerEvent(GUIManager* o, GUIEvent* gev);
#define GUIObject_TriggerEvent(o, gev) GUIObject_TriggerEvent_(&(o)->header, gev)
void GUIObject_TriggerEvent_(GUIHeader* o, GUIEvent* gev);
void GUIManager_BubbleEvent(GUIManager* gm, GUIObject* target, GUIEvent* gev);

void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev);
void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev);
void GUIManager_HandleKeyInput(GUIManager* gm, InputState* is, InputEvent* iev);



void guiSetClientSize(GUIObject* go, Vector2 cSize);
Vector2 guiGetClientSize(GUIObject* go);
Vector2 guiRecalcClientSize(GUIObject* go);





#include "ui/configLoader.h"





#endif // __EACSMB_GUI_H__
