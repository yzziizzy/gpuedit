
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
	t.vt = (InputEventHandler**)(data + vtoffset);
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


void InputFocusStack_Dispatch(InputFocusStack* stack, InputEvent* ev) {
	if(VEC_LEN(&stack->stack) == 0) return;
	
	InputFocusTarget* h = &VEC_TAIL(&stack->stack);
	
#define CALLIF(x) if(*h->vt && (*h->vt)->x) ((*h->vt)->x)(ev, h->data) 
	
	CALLIF(all);
	
	switch(ev->type) {
		case EVENT_KEYDOWN: CALLIF(keyDown); break;
		case EVENT_KEYUP: CALLIF(keyUp); break;
		case EVENT_TEXT: CALLIF(keyText); break;
		case EVENT_MOUSEDOWN: CALLIF(mouseDown); break;
		case EVENT_MOUSEUP: CALLIF(mouseUp); break;
		case EVENT_CLICK: CALLIF(click); break;
		case EVENT_DOUBLECLICK: CALLIF(doubleClick); break;
		case EVENT_DRAGSTART: CALLIF(dragStart); break;
		case EVENT_DRAGSTOP: CALLIF(dragStop); break;
		case EVENT_DRAGMOVE: CALLIF(dragMove); break;
		case EVENT_MOUSEMOVE: CALLIF(mouseMove); break;
		case EVENT_MOUSEENTER: CALLIF(mouseEnter); break;
		case EVENT_MOUSELEAVE: CALLIF(mouseLeave); break;
		case EVENT_GAINFOCUS: CALLIF(gainFocus); break;
		case EVENT_LOSEFOCUS: CALLIF(loseFocus); break;
		default:
			fprintf(stderr, "!!! Unknown event in InputFocusStack_Dispatch: %d\n", ev->type);
			break;
	}

#undef CALLIF
	
}

void InputFocusStack_DispatchPerFrame(InputFocusStack* stack, InputState* is, float frameSpan) {
	
	if(VEC_LEN(&stack->stack) == 0) return;
	
	InputFocusTarget* h = &VEC_TAIL(&stack->stack);
	if(*h->vt && (*h->vt)->perFrame) {
		((*h->vt)->perFrame)(is, frameSpan, h->data); 
	}
}


// gets a human-readable string for any specific key, such as "Control_R" for the right control key
char* InputEvent_GetKeyDescription(InputEvent* iev) {
	char* xs = XKeysymToString((KeySym)iev->keysym);
	char* s = strdup(xs);
	XFree(xs);
	
	return s;
}



