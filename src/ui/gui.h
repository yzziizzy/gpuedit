#ifndef __gputk_gui_H__
#define __gputk_gui_H__

#include <stdatomic.h>

#include "../gui_deps.h"

#include "gui_settings.h"
#include "commands.h"

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

struct GUIHeader;
typedef struct GUIHeader GUIHeader;
struct GUIManager;

struct GUIRenderParams;
typedef struct GUIRenderParams GUIRenderParams;


struct gui_vtbl {
	void (*UpdatePos)(GUIHeader* go, GUIRenderParams* grp, PassFrameParams* pfp);
	void (*Render)(GUIHeader* go, PassFrameParams* pfp);
	void (*Delete)(GUIHeader* go);
	void (*Reap)(GUIHeader* go);
	void (*Resize)(GUIHeader* go, Vector2 newSz); // exterior size
	Vector2 (*GetClientSize)(GUIHeader* go);
	void    (*SetClientSize)(GUIHeader* go, Vector2 cSize); // and force exterior size to match
	Vector2 (*RecalcClientSize)(GUIHeader* go); // recalc client size from the client children and call SetClientSize
	GUIHeader* (*HitTest)(GUIHeader* go, Vector2 testPos);
	GUIHeader* (*FindChild)(GUIHeader* h, char* name);
	
	void (*AddClient)(GUIHeader* parent, GUIHeader* child);
	void (*RemoveClient)(GUIHeader* parent, GUIHeader* child);
	Vector2 (*SetScrollPct)(GUIHeader* go, Vector2 pct); // returns the final value
	Vector2 (*SetScrollAbs)(GUIHeader* go, Vector2 absPos);
	
	int (*HandleCommand)(GUIHeader* h, GUI_Cmd* cmd);
};


struct GUIEvent;
typedef struct GUIEvent GUIEvent;
typedef void (*GUI_EventHandlerFn)(GUIHeader*, GUIEvent*);

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
	char* userType;
	
	double eventTime;
	Vector2 eventPos;
	GUIHeader* originalTarget;
	GUIHeader* currentTarget;
	
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


typedef int  (*GUI_OnClickFn)(GUIHeader* go, Vector2 clickPos);
typedef void (*GUI_OnMouseEnterFn)(GUIEvent* e);
typedef void (*GUI_OnMouseLeaveFn)(GUIEvent* e);


#define GUIMOUSECURSOR_ARROW 0x01
#define GUIMOUSECURSOR_TEXT  0x02
#define GUIMOUSECURSOR_WAIT  0x03

#define GUI_MAXIMIZE_X      0x0001
#define GUI_MAXIMIZE_Y      0x0002
#define GUI_NO_CLIP         0x0004
#define GUI_SIZE_TO_CONTENT 0x0008 // NYI: (gravity makes this very complicated) automatic, based on children
#define GUI_AUTO_SIZE       0x0010 // manual flag, for the bottom level
#define GUI_CHILD_TABBING   0x0020 // grab tab events and cycle focused elements


struct GUIHeader {
	struct GUIManager* gm;
	GUIHeader* parent;
	struct gui_vtbl* vt;
	struct GUIEventHandler_vtbl* event_vt;
	
	VEC(struct {
		enum GUIEventType type;
		GUI_EventHandlerFn cb;
	}) dynamicHandlers;
	
	char* name; // used by config loader atm

	// fallback for easy hit testing
	VEC(GUIHeader*) children;
	
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
	
	uint16_t cmdElementType; // which category of commands this element should respond to
	
	int cursor;
	int tabStop; // the tab stop of this element within its parent hierarchy
	
	// data about tabbing among children of this element
	int currentTabStop;
	VEC(GUIHeader*) tabStopCache;
	
};



// Animations
#include "ui/animations/pulse.h"



// GUI elements
#include "window.h"
#include "text.h"
#include "textf.h"
#include "button.h"
#include "scrollWindow.h"
#include "simpleWindow.h"
#include "image.h"
#include "imgButton.h"
#include "tree.h"
#include "edit.h"
#include "selectBox.h"
#include "slider.h"
#include "columnLayout.h"
#include "gridLayout.h"
#include "tabBar.h"
#include "tabControl.h"
#include "formControl.h"
#include "monitors.h"
#include "debugAdjuster.h"
#include "structAdjuster.h"
#include "performanceGraph.h"
#include "fileBrowser.h"


/*
The general idea is this:
The gui is held in a big tree. The tree is walked depth-first from the bottom up, resulting in
a mostly-sorted list. A sort is then run on z-index to ensure proper order. The list is then
rendered all at once through a single unified megashader using the geometry stage to expand
points into quads.

*/
typedef struct GUIManager {
	int maxInstances; // limited by vmem ring buffer settings
	PCBuffer instVB;
	GLuint vao;
	
	int elementCount;
	int elementAlloc;
	GUIUnifiedVertex* elemBuffer;
	
	Vector2i screenSize;
	
	GUIHeader* root;
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
	
	char useSoftCursor;
	char* softCursorName;
	Vector2 softCursorSize;
	
	RING(GUIHeader*) focusStack;
	
	char nextCmdFlagBit;
	VEC(GUI_CmdElementInfo) cmdElements;
	HT(int) cmdElementLookup;
	HT(uint16_t) cmdModeLookup;
//	HT(uint32_t) cmdNameLookup;
	HT(uint32_t) cmdFlagLookup;
	
	VEC(GUI_Cmd) cmdList; // temporary, until a struct-keyed hash table is added to sti
	
		
	struct {
		GUIFont* font;
		float fontSize;
		struct Color4 textColor;
		struct Color4 editBgColor;
		struct Color4 editBorderColor;
		struct Color4 editTextColor;
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
		struct Color4 outlineCurrentLineBorderColor;
		struct Color4 selectedItemTextColor;
		struct Color4 selectedItemBgColor;
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
		Vector2       selectSize;
		struct Color4 trayBgColor;
		float charWidth_fw;
		float lineHeight_fw;
		GUIFont* font_fw;
		float fontSize_fw;
		struct Color4 statusBarBgColor;
		struct Color4 statusBarTextColor;
		
		
		// TODO: font name, size 
	} defaults;
	
	
	json_value_t* templates;
	
	// temp 
	GLuint fontAtlasID;
	GLuint atlasID;
	
	VEC(GLuint64) texHandles;
	
	GUI_GlobalSettings* gs;
	
} GUIManager;


typedef union {
	GUIHeader header;
} GUIObject;


void GUIManager_init(GUIManager* gm, GUI_GlobalSettings* gs);
void GUIManager_initGL(GUIManager* gm);
GUIManager* GUIManager_alloc(GUI_GlobalSettings* gs);


RenderPass* GUIManager_CreateRenderPass(GUIManager* gm);
PassDrawable* GUIManager_CreateDrawable(GUIManager* gm);

void GUIManager_SetMainWindowTitle(GUIManager* gm, char* title);
void GUIManager_SetCursor(GUIManager* gm, int cursor);



void GUIManager_updatePos(GUIManager* gm, PassFrameParams* pfp);
void GUIManager_Reap(GUIManager* gm);


GUIHeader* GUIHeader_hitTest(GUIHeader* go, Vector2 testPos);
GUIHeader* GUIManager_hitTest(GUIManager* gm, Vector2 testPos);
// 
// void GUIHeader_triggerClick(GUIHeader* go, GUIEvent* e); 
void GUIHeader_triggerClick(GUIHeader* go, Vector2 testPos);
GUIHeader* GUIManager_triggerClick(GUIManager* gm, Vector2 testPos);


// focus stack
GUIHeader* GUIManager_getFocusedObject(GUIManager* gm);
//#define GUIManager_pushFocusedObject(gm, o) GUIManager_pushFocusedObject_(gm, &(o)->header)
void GUIManager_pushFocusedObject(GUIManager* gm, GUIHeader* h);
GUIHeader* GUIManager_popFocusedObject(GUIManager* gm);


void GUIResize(GUIHeader* gh, Vector2 newSz);


void guiTriggerClick(GUIEvent* e); 


// USE THIS ONE. virtual function.
#define GUI_AddClient(p, o) GUIHeader_AddClient((p) ? &((p)->header) : NULL, &(o)->header)
void GUIHeader_AddClient(GUIHeader* parent, GUIHeader* o);

#define GUI_RemoveClient(p, o) GUIHeader_RemoveClient((p) ? &((p)->header) : NULL, &(o)->header)
void GUIHeader_RemoveClient(GUIHeader* parent, GUIHeader* o);


// NOT this one. direct child addition
#define GUI_RegisterObject(p, o) GUIHeader_RegisterObject((p) ? &(p)->header : NULL, &(o)->header)
void GUIHeader_RegisterObject(GUIHeader* parent, GUIHeader* o);

#define GUI_UnregisterObject(o) GUIHeader_UnregisterObject(&(o)->header)
void GUIHeader_UnregisterObject(GUIHeader* o);



// Delete marks things to be reaped later. It removes objects from the root tree.
#define GUI_Delete(o) GUIHeader_Delete(&(o)->header)
void GUIHeader_Delete(GUIHeader* h);

// Reap is for garbage collection. 
// It happens at a separate phase and does not depend on object relations. 
#define GUI_Reap(o) GUIHeader_Reap(&(o)->header)
void GUIHeader_Reap(GUIHeader* h);

/*#define GUI_AddClient(p, c) GUIHeader_AddClient_(&(p)->header, &(c)->header)
void GUIHeader_AddClient_(GUIHeader* parent, GUIHeader* client);

#define GUI_RemoveClient(p, c) GUIHeader_RemoveClient_(&(p)->header, &(c)->header)
void GUIHeader_RemoveClient_(GUIHeader* parent, GUIHeader* client);
*/

#define GUI_SetScrollPct(o, pct) GUIHeader_SetScrollPct(&(o)->header, pct)
Vector2 GUIHeader_SetScrollPct(GUIHeader* go, Vector2 pct);

#define GUI_SetScrollAbs(o, abs) GUIHeader_SetScrollAbs(&(o)->header, abs)
Vector2 GUIHeader_SetScrollAbs(GUIHeader* go, Vector2 absPos);

#define GUI_FindChild(p, n) GUIHeader_FindChild((p) ? &((p)->header) : NULL, n)
GUIHeader* GUIHeader_FindChild(GUIHeader* obj, char* name);



/*
NOTE:
These functions are for temporal, dynamic event handlers that must be added
and removed at run time.

They are NOT for general event handling in GUI elements. Use the vtable for that.
*/
#define GUI_AddHandler(o, t, c) GUIHeader_AddHandler(&(o)->header, t, c)
void GUIHeader_AddHandler(GUIHeader* h, enum GUIEventType type, GUI_EventHandlerFn cb);

#define GUI_RemoveHandler(o, t, c) GUIHeader_RemoveHandler(&(o)->header, t, c)
void GUIHeader_RemoveHandler(GUIHeader* h, enum GUIEventType type, GUI_EventHandlerFn cb);



// USE THIS ONE to send an event into the system.
// gev can be allocated on the stack.
void GUIManager_BubbleEvent(GUIManager* gm, GUIHeader* target, GUIEvent* gev);
void GUIManager_BubbleUserEvent(GUIManager* gm, GUIHeader* target, char* ev);

// TriggerEvent DOES NOT BUBBLE. It only calls the virtual functions on the object.
// gev can be allocated on the stack.
void GUIManager_TriggerEvent(GUIManager* o, GUIEvent* gev);
#define GUI_TriggerEvent(o, gev) GUIHeader_TriggerEvent(&(o)->header, gev)
void GUIHeader_TriggerEvent(GUIHeader* o, GUIEvent* gev);

void GUIHeader_RegenTabStopCache(GUIHeader* parent);

#define GUI_NextTabStop(o) GUIHeader_NextTabStop(&((o)->header));
GUIHeader* GUIHeader_NextTabStop(GUIHeader* h);

void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev);
void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev);
void GUIManager_HandleKeyInput(GUIManager* gm, InputState* is, InputEvent* iev);

GUIHeader* GUIManager_SpawnTemplate(GUIManager* gm, char* name);



void guiSetClientSize(GUIHeader* go, Vector2 cSize);
Vector2 guiGetClientSize(GUIHeader* go);
Vector2 guiRecalcClientSize(GUIHeader* go);


// debugging
void gui_debugFileDumpVertexBuffer(GUIManager* gm, char* filePrefix, int fileNum);


#include "configLoader.h"





#endif // __gputk_gui_h__
