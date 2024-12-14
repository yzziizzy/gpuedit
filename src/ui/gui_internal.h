#ifndef __EACSMB_gui_internal_h__
#define __EACSMB_gui_internal_h__

#include <stdarg.h>

// this file is for gui element implementations, not for general outside usage


typedef struct GUIRenderParams {
	Vector2 offset; // parent-defined absolute from the top left of the screen
	Vector2 size; // of the parent's client area that the child lives in
	AABB2 clip; // absolute clipping region
	float baseZ; // accumulated absolute Z from the parent
} GUIRenderParams;



int GUI_HandleCursor_(GUIManager* gm, GUICursorData* cd, GUIString* str, GUIEvent* e);
#define GUI_HandleCursor(...) GUI_HandleCursor_(gm, __VA_ARGS__)

int GUI_HandleTextInput_(GUIManager* gm, GUICursorData* cd, GUIString* str, GUIEvent* e);
#define GUI_HandleTextInput(...) GUI_HandleTextInput_(gm, __VA_ARGS__)



GUIUnifiedVertex* GUIWindow_checkElemBuffer(GUIWindow* w, int count);
GUIUnifiedVertex* GUIManager_reserveElements(GUIManager* gm, int count);
void GUIManager_copyElements(GUIManager* gm, GUIUnifiedVertex* verts, int count);



static inline AABB2 gui_clipTo(AABB2 parent, AABB2 child) {
	return (AABB2){
		.min.x = fmax(parent.min.x, child.min.x),
		.min.y = fmax(parent.min.y, child.min.y),
		.max.x = fmin(parent.max.x, child.max.x),
		.max.y = fmin(parent.max.y, child.max.y),
	};
}


static inline void GUIString_Init(GUIString* s) {
	s->len = 0;
	s->alloc = 64;
	s->data = malloc(s->alloc * sizeof(*s->data));
	s->data[0] = 0;
}


#define GUI_TEXT_ALIGN_LEFT 0x00
#define GUI_TEXT_ALIGN_RIGHT 0x01
#define GUI_TEXT_ALIGN_CENTER 0x02



void GUI_Image_(GUIManager* gm, Vector2 tl, Vector2 sz, char* name);
#define GUI_Image(a,b,c) GUI_Image_(gm, a,b,c)

// draws a single character from its font origin point
void GUI_Char_(GUIManager* gm, int c, Vector2 origin, char* fontName, float size, Color4* color);
#define GUI_Char(a,b,c,d,e) GUI_Char_(gm, a,b,c,d,e) 
void GUI_CharFont_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color);
#define GUI_CharFont(a,b,c,d,e) GUI_CharFont_(gm, a,b,c,d,e) 
// lacks a drawMode guard
void GUI_CharFont_NoGuard_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color);
#define GUI_CharFont_NoGuard(a,b,c,d,e) GUI_CharFont_NoGuard_(gm, a,b,c,d,e) 

// no wrapping
void GUI_TextLine_(
	GUIManager* gm, 
	char* text, 
	size_t textLen, 
	Vector2 tl, 
	char* fontName, 
	float size, 
	Color4* color
);
#define GUI_TextLine(a,b, c,d, f,g) GUI_TextLine_(gm, a,b, c,d, f,g)

// no wrapping
void GUI_TextLineCentered_(
	GUIManager* gm, 
	char* text, 
	size_t textLen, 
	Vector2 tl, 
	Vector2 sz, 
	char* fontName, 
	float size, 
	Color4* color
);

#define GUI_TextLineCentered(...) GUI_TextLineCentered_(gm, __VA_ARGS__)


// no wrapping
void GUI_Printf_(
	GUIManager* gm,  
	Vector2 tl, 
	char* fontName, 
	float size, 
	Color4* color,
	char* fmt,
	...
);
#define GUI_Printf(...) GUI_Printf_(gm, __VA_ARGS__)



#define  GUI_CharFromPixelF(a,b,c,d,e) GUI_CharFromPixelF_(gm, a,b,c,d,e)
float GUI_CharFromPixelF_(
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt,
	ssize_t maxChars,
	float pixelOffset
);


void GUI_Double_(GUIManager* gm, double d, int precision, Vector2 tl, char* fontName, float size, Color4* color);
#define GUI_Double(...) GUI_Double_(gm, __VA_ARGS__)
void GUI_Integer_(GUIManager* gm, int64_t i, Vector2 tl, char* fontName, float size, Color4* color);
#define GUI_Integer(...) GUI_Integer_(gm, __VA_ARGS__)

void gui_drawBox(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	AABB2* clip, 
	float z, 
	Color4* color
);

void gui_drawBoxBorder(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	AABB2* clip, 
	float z, 
	Color4* bgColor,
	float borderWidth,
	Color4* borderColor
);


void GUI_Triangle_(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height, 
	float rotation,
	Color4* bgColor
);
#define GUI_Triangle(...) GUI_Triangle_(gm, __VA_ARGS__)

void gui_drawTriangleBorder(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height, 
	float rotation,
	AABB2* clip, 
	float z, 
	Color4* bgColor,
	float borderWidth,
	Color4* borderColor
);

void gui_drawImg(
	GUIManager* gm, 
	char* name, 
	Vector2 tl, 
	Vector2 sz, 
	AABB2* clip, 
	float z
);




void gui_drawCharacter(
	GUIManager* gm,
	Vector2 tl,
	Vector2 sz,
	AABB2* clip,
	float z,
	int c,
	struct Color4* color,
	GUIFont* font,
	float fontsize
);

// stops on linebreak
void gui_drawTextLine(
	GUIManager* gm,
	Vector2 tl,  
	Vector2 sz,  
	AABB2* clip,  
	struct Color4* color,
	float z,
	char* txt, 
	size_t charCount
);

void gui_drawTextLineAdv(
	GUIManager* gm,
	Vector2 tl,
	Vector2 sz,
	AABB2* clip,
	struct Color4* color,
	GUIFont* font,
	float fontsize,
	int align,
	float z,
	char* txt, 
	size_t charCount
);

int gui_charFromPixel (
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt, 
	float pixelOffset
);

float gui_getTextLineWidth(
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt, 
	size_t charCount
);


float gui_getDefaultUITextWidth(
	GUIManager* gm,
	char* txt, 
	size_t maxChars
);

void gui_drawVCenteredTextLine(
	GUIManager* gm,
	Vector2 tl,  
	Vector2 sz,
	AABB2* clip,
	struct Color4* color,
	float z,
	char* txt, 
	size_t charCount
);


void GUI_SetHot_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*));
#define  GUI_SetHot(a) GUI_SetHot_(gm, a, NULL, NULL)
void GUI_SetActive_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*));
#define  GUI_SetActive(a) GUI_SetActive_(gm, a, NULL, NULL)
void* GUI_GetData_(GUIManager* gm, void* id);
#define GUI_GetData(b) GUI_GetData_(gm, b)
void GUI_SetData_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*));
#define GUI_SetData(a,b,c) GUI_SetData_(gm, a,b,c)

int GUI_InputAvailable_(GUIManager*gm);
#define GUI_InputAvailable() GUI_InputAvailable_(gm)
void GUI_CancelInput_(GUIManager*gm);
#define GUI_CancelInput() GUI_CancelInput_(gm)

// the last known position of the mouse
Vector2 GUI_MousePos_(GUIManager* gm);
#define GUI_MousePos() GUI_MousePos_(gm)

// only valid when the mouse is dragging
Vector2 GUI_DragStartPos_(GUIManager* gm, int button);
#define GUI_DragStartPos(b) GUI_DragStartPos_(gm, b)

// may output garbage on non-mouse events
Vector2 GUI_EventPos_(GUIManager* gm);
#define GUI_EventPos() GUI_EventPos_(gm)


int GUI_MouseInside_(GUIManager* gm, Vector2 tl, Vector2 sz);
#define GUI_MouseInside(a, b) GUI_MouseInside_(gm, a, b)
int GUI_MouseWentUp_(GUIManager* gm, int button);
#define GUI_MouseWentUp(a) GUI_MouseWentUp_(gm, a)
int GUI_MouseWentDown_(GUIManager* gm, int button);
#define GUI_MouseWentDown(a) GUI_MouseWentDown_(gm, a)
int GUI_MouseWentUpAnywhere_(GUIManager* gm, int button);
#define GUI_MouseWentUpAnywhere(a) GUI_MouseWentUpAnywhere_(gm, a) 
int GUI_MouseWentDownAnywhere_(GUIManager* gm, int button);
#define GUI_MouseWentDownAnywhere(a) GUI_MouseWentDownAnywhere_(gm, a)

float GUI_GetScrollDist_(GUIManager* gm);
#define GUI_GetScrollDist() GUI_GetScrollDist_(gm)


// sets the current clipping region, respecting the current window
void GUI_PushClip_(GUIManager* gm, Vector2 tl, Vector2 sz);
#define GUI_PushClip(a, b) GUI_PushClip_(gm, a, b)

void GUI_PopClip_(GUIManager* gm);
#define GUI_PopClip() GUI_PopClip_(gm)

// create a new window, push it to the stack, and set it current
void GUI_BeginWindow_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, float z, unsigned long flags);
#define GUI_BeginWindow(a,b,c,d,e) GUI_BeginWindow_(gm, a, b, c, d, e)

// pop the window stack and set the previous window to be current
void GUI_EndWindow_(GUIManager* gm);
#define GUI_EndWindow() GUI_EndWindow_(gm)

/*  _______________
  //              //
 //   Controls   //
//______________*/


// returns true if clicked
int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, char* text, GUIButtonOpts* o);
#define GUI_Button(a,b,c) GUI_Button_(gm, a,b,c, &gm->defaults.GUIButtonOpts)

int GUI_RectButton_(
	GUIManager* gm, 
	void* id, 
	Vector2 tl, 
	Vector2 sz, 
	char* text, 
	char* fontName, 
	float fontSize, 
	Color4* fontColor, 
	Color4 colors[3]
);
#define GUI_RectButton(...) GUI_RectButton_(gm, __VA_ARGS__)

// returns true if toggled on
int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, char* text, int* state, GUIToggleButtonOpts* o);
#define GUI_ToggleButton(a,b,c,d) GUI_ToggleButton_(gm, a,b,c,d, &gm->defaults.GUIToggleButtonOpts)

// returns true if checked
int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, char* label, int* state, GUICheckboxOpts* o);
#define GUI_Checkbox(a,b,c,d) GUI_Checkbox_(gm, a,b,c,d, &gm->defaults.GUICheckboxOpts)

// returns true if *this* radio button is active
int GUI_RadioBox_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault, GUIRadioBoxOpts* o);
#define GUI_RadioBox(a, b, c, d, e) GUI_RadioBox_(gm, a,b,c,d,e, &gm->defaults.GUIRadioBoxOpts)

// returns 1 on change
int GUI_FloatSlider_(GUIManager* gm, void* id, Vector2 tl, float min, float max, float incr, float* value, GUIFloatSliderOpts* o);
#define GUI_FloatSlider(a,b,c,d,e,f) GUI_FloatSlider_(gm, a,b,c,d,e,f, &gm->defaults.GUIFloatSliderOpts)

// returns 1 on change
int GUI_IntSlider_(GUIManager* gm, void* id, Vector2 tl, long min, long max, long* value, GUIIntSliderOpts* o);
#define GUI_IntSlider(a,b,c,d,e) GUI_IntSlider_(gm, a,b,c,d,e, &gm->defaults.GUIIntSliderOpts)

// returns 1 when the value changes _due to this control_
int GUI_OptionSlider_(GUIManager* gm, void* id, Vector2 tl, char** options, int* selectedOption, GUIOptionSliderOpts* o);
#define GUI_OptionSlider(a,b,c,d) GUI_OptionSlider_(gm, a,b,c,d, &gm->defaults.GUIOptionSliderOpts)

// filter all input before accepting it
void GUI_Edit_SetFilter_(GUIManager* gm, void* id, GUIEditFilterFn fn, void* data);
#define GUI_Edit_SetFilter(a, b, c) GUI_Edit_SetFilter_(gm, a, b, c)

// synthesizes an input event for the given edit control
int GUI_Edit_Trigger_(GUIManager* gm, void* id, GUIString* str, int c);
#define GUI_Edit_Trigger(a, b, c) GUI_Edit_Trigger_(gm, a, b, c)

// returns true on a change
int GUI_Edit_(GUIManager* gm, void* id, Vector2 tl, float width, GUIString* s, GUIEditOpts* o);
#define GUI_Edit(a,b,c,d) GUI_Edit_(gm, a,b,c,d, &gm->defaults.GUIEditOpts)

// returns true on a change
int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, float width, long* num, GUIIntEditOpts* o);
#define GUI_IntEdit(a,b,c) GUI_IntEdit_(gm, a,b,c, &gm->defaults.GUIIntEditOpts)

// returns true on a change
int GUI_SelectBox_(GUIManager* gm, void* id, Vector2 tl, float width, char** options, int* selectedOption, GUISelectBoxOpts* o);
#define GUI_SelectBox(a,b,c,d,e) GUI_SelectBox_(gm, a,b,c,d,e, &gm->defaults.GUISelectBoxOpts)

// outline of a box with transparent center
void GUI_Box_(GUIManager* gm, Vector2 tl, Vector2 sz, float width, Color4* borderColor);
#define GUI_Box(a, b, c, d) GUI_Box_(gm, a, b, c, d)

// filled outline
void GUI_BoxFilled_(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	float width, 
	Color4* borderColor, 
	Color4* bgColor
);
#define GUI_BoxFilled(a, b, c, d, e) GUI_BoxFilled_(gm, a, b, c, d, e)

// solid rectangle with no border
#define GUI_Rect(tl, sz, color) GUI_BoxFilled_(gm, tl, sz, 0, &(Color4){0,0,0,0}, color)

void GUI_CircleFilled_(
	GUIManager* gm, 
	Vector2 center, 
	float radius, 
	float borderWidth, 
	Color4* borderColor, 
	Color4* bgColor
);

#define GUI_CircleFilled(a,b,c,d,e) GUI_CircleFilled_(gm, a,b,c,d,e);

// no wrapping
void GUI_TextLine_(
	GUIManager* gm, 
	char* text, 
	size_t textLen, 
	Vector2 tl, 
	char* fontName, 
	float size, 
	Color4* color
);

// repects the current window and clipping region
int GUI_PointInBoxV_(GUIManager* gm, Vector2 tl, Vector2 size, Vector2 testPos);
#define GUI_PointInBoxV(a, b, c) GUI_PointInBoxV_(gm, a, b, c)
int GUI_PointInBox_(GUIManager* gm, AABB2 box, Vector2 testPos);
#define GUI_PointInBox(a, b) GUI_PointInBox_(gm, a, b)

// just straight math
int GUI_PointInBoxVABS(Vector2 tl, Vector2 size, Vector2 testPos);






#endif // __EACSMB_gui_internal_h__
