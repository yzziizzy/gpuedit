#ifndef __EACSMB__input_h__
#define __EACSMB__input_h__

#include "sti/sti.h"
#include "common_math.h"


// raw input states
#define IS_KEYDOWN    0x01 // is it down now
#define IS_KEYPRESSED 0x02 // was the key pressed ever
#define IS_CONTROL    0x04
#define IS_SHIFT      0x08
#define IS_ALT        0x10
#define IS_TUX        0x20 // aka "windows key"
// #define IS_MOD2       0x40  // numlock
// #define IS_MOD3       0x80 // unused


enum InputMode {
	CLICK_MODE,
	DRAG_MODE
	
};

enum InputEventType {
	EVENT_KEYDOWN, // 0
	EVENT_KEYUP, // 1
	EVENT_TEXT, // 2
	EVENT_MOUSEDOWN, // 3
	EVENT_MOUSEUP, // 4
	EVENT_CLICK,
	EVENT_DOUBLECLICK,
	EVENT_DRAGSTART,
	EVENT_DRAGSTOP,
	EVENT_DRAGMOVE,
	EVENT_MOUSEMOVE,
	EVENT_MOUSEENTER, // enter and leave *the application window*, not some arbitrary region.
	EVENT_MOUSELEAVE, // may happen during a drag
	EVENT_GAINFOCUS,
	EVENT_LOSEFOCUS
};


struct InputEvent;

typedef struct InputState {

	// settings
	float doubleClickTime;
	float dragMinDist;
	
	// state
	double lastClickTime;
	double lastMoveTime;
	
	Vector2i lastPressPosPixels;
	Vector2 lastPressPosNorm;
	double lastPressTime;
	
	Vector2 lastClickPos;
	Vector2 lastCursorPos;
	Vector2i lastCursorPosPixels;

	char clickButton;
	char buttonUp;
	char buttonDown;
	char inDrag;
	
	unsigned char keyState[256];
//	double keyStateChanged[256];
	
	VEC(struct InputEvent*) events;
	
	enum InputMode mode;
	
} InputState;


// for any event, fields not related are undefined
typedef struct {
	char type; 
	
	union {
		char button; 
		char character; // translated after mod keys
	};
	unsigned short kbmods; // control, alt, etc. uses "raw input states" macros
	
	unsigned int keysym;
	
	double time;
	
	Vector2i intPos; 
	Vector2 normPos; 
	
	Vector2i intDragStart;
	Vector2 normDragStart;
	
	InputState* is;
} InputEvent;

struct InputFocusStack;
// return 0 to stop bubbling
typedef int (*input_event_fn)(InputEvent*, void*);
typedef int (*input_per_frame_fn)(InputState*, float, void*);

typedef struct InputEventHandler {
	input_event_fn all;
	
	input_event_fn keyDown;
	input_event_fn keyUp;
	input_event_fn keyText;
	
	input_event_fn mouseDown;
	input_event_fn mouseUp;
	input_event_fn click;
	input_event_fn doubleClick;
	input_event_fn mouseMove;
	input_event_fn dragMove;
	input_event_fn dragStart;
	input_event_fn dragStop;
	
	// enter and leave the application window as a whole. 
	// may happen during a drag
	input_event_fn mouseEnter;
	input_event_fn mouseLeave;
	
	// misc: lost focus. gained focus.
	input_event_fn loseFocus; // TODO: not implemented
	input_event_fn gainFocus;
	
	// TODO: per-frame callback with state, including frame time
	input_per_frame_fn perFrame;
	
	struct InputFocusStack* stack;
} InputEventHandler;



typedef struct InputFocusTarget {
	void* data;
	InputEventHandler** vt;
} InputFocusTarget;

typedef struct InputFocusStack {
	
	VEC(InputFocusTarget) stack;
	
	
} InputFocusStack;


// effective usage with macro:
// InputFocusStack_PushTarget(InputFocusStack* stack, YourType* data, vtable_field_name_in_data);
#define InputFocusStack_PushTarget(stack, data, field) \
	_InputFocusStack_PushTarget((stack), (void*)(data), (void*)(&(data)->field) - (void*)(data))

void _InputFocusStack_PushTarget(InputFocusStack* stack, void* data, ptrdiff_t vtoffset);
void InputFocusStack_PushTarget2(InputFocusStack* stack, void* data, InputEventHandler** ptr);

int InputFocusTarget_Dispatch(InputFocusTarget* t , InputEvent* ev);
void InputFocusStack_RevertTarget(InputFocusStack* stack);
int InputFocusStack_Dispatch(InputFocusStack* stack, InputEvent* ev);
int InputFocusStack_DispatchPerFrame(InputFocusStack* stack, InputState* is, float frameSpan);

void clearInputState(InputState* st);

char* InputEvent_GetKeyDescription(InputEvent* iev);

#endif // __EACSMB__input_h__
