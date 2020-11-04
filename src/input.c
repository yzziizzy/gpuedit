
#include <X11/X.h>
#include <X11/Xlib.h>

#include "input.h"


// very outdated
void clearInputState(InputState* st) {
	int i;
	
	for(i = 0; i < 256; i++) {
		st->keyState[i] &= IS_KEYDOWN;
	}
	
	//st->clickPos.y = -1;
//	st->clickPos.x = -1;
	st->clickButton = 0;
	st->buttonUp = 0;
	st->buttonDown = 0;
	
}



// effective usage with macro:
// InputFocusStack_PushTarget(InputFocusStack* stack, YourType* data, vtable_field_name_in_data);
void _InputFocusStack_PushTarget(InputFocusStack* stack, void* data, ptrdiff_t vtoffset) {
	//InputFocusStack_PushTarget2(stack, data, (data + vtoffset));
}

void InputFocusStack_PushTarget2(InputFocusStack* stack, void* data, InputEventHandler** ptr) {
	
	InputFocusTarget t;
	InputFocusTarget* t2;
	
	if(VEC_LEN(&stack->stack)) {
		t2 = &VEC_TAIL(&stack->stack);
		(*t2->vt)->stack = stack;
		if((*t2->vt)->loseFocus) {
			(*t2->vt)->loseFocus(NULL, t2->data);
		}
	}
	
	t.data = data;
	t.vt = ptr;
	VEC_PUSH(&stack->stack, t);
	
	if(*t.vt) {
		(*t.vt)->stack = stack;
		if((*t.vt)->gainFocus) {
			(*t.vt)->gainFocus(NULL, t.data);
		}
	}
}



void InputFocusStack_RevertTarget(InputFocusStack* stack) {
	
	// notify of losing focus
	InputFocusTarget* t = &VEC_TAIL(&stack->stack);
	if((*t->vt)->loseFocus) {
		(*t->vt)->loseFocus(NULL, t->data);
	}
	
	VEC_POP1(&stack->stack);
	
	// notify of gaining focus
	t = &VEC_TAIL(&stack->stack);
	if((*t->vt)->gainFocus) {
		(*t->vt)->gainFocus(NULL, t->data);
	}
}


int InputFocusStack_Dispatch(InputFocusStack* stack, InputEvent* ev) {
	int ret = 99;
	if(VEC_LEN(&stack->stack) == 0) return NULL;
	
	for(int i = VEC_LEN(&stack->stack) - 1; i >= 0; i--) {
		InputFocusTarget* h = &VEC_ITEM(&stack->stack, i);
	
		ret = InputFocusTarget_Dispatch(h, ev);
		if(ret == 0) return 0; 
	}
	
	return ret;
}


int InputFocusTarget_Dispatch(InputFocusTarget* t , InputEvent* ev) {
	
#define CALLIF(x) if(*t->vt && (*t->vt)->x) return ((*t->vt)->x)(ev, t->data) 
	
	CALLIF(all);
	
	switch(ev->type) {
		case EVENT_KEYDOWN: CALLIF(keyDown);
		case EVENT_KEYUP: CALLIF(keyUp);
		case EVENT_TEXT: CALLIF(keyText);
		case EVENT_MOUSEDOWN: CALLIF(mouseDown);
		case EVENT_MOUSEUP: CALLIF(mouseUp);
		case EVENT_CLICK: CALLIF(click);
		case EVENT_DOUBLECLICK: CALLIF(doubleClick);
		case EVENT_DRAGSTART: CALLIF(dragStart);
		case EVENT_DRAGSTOP: CALLIF(dragStop);
		case EVENT_DRAGMOVE: CALLIF(dragMove);
		case EVENT_MOUSEMOVE: CALLIF(mouseMove);
		case EVENT_MOUSEENTER: CALLIF(mouseEnter);
		case EVENT_MOUSELEAVE: CALLIF(mouseLeave);
		case EVENT_GAINFOCUS: CALLIF(gainFocus);
		case EVENT_LOSEFOCUS: CALLIF(loseFocus);
		default:
			fprintf(stderr, "!!! Unknown event in InputFocusTarget_Dispatch: %d\n", ev->type);
			return -1;
	}

#undef CALLIF
}



int InputFocusStack_DispatchPerFrame(InputFocusStack* stack, InputState* is, float frameSpan) {
	int ret = 99;
	if(VEC_LEN(&stack->stack) == 0) return NULL;
	
	for(int i = VEC_LEN(&stack->stack) - 1; i >= 0; i--) {
		InputFocusTarget* h = &VEC_ITEM(&stack->stack, i);
	
		if(*h->vt && (*h->vt)->perFrame) {
			ret = ((*h->vt)->perFrame)(is, frameSpan, h->data); 
		}
		if(ret == 0) return 0; 
	}
	
	return ret;
}


// gets a human-readable string for any specific key, such as "Control_R" for the right control key
char* InputEvent_GetKeyDescription(InputEvent* iev) {
	char* xs = XKeysymToString((KeySym)iev->keysym);
	char* s = strdup(xs);
	XFree(xs);
	
	return s;
}



