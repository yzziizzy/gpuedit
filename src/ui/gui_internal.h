#ifndef __EACSMB_gui_internal_h__
#define __EACSMB_gui_internal_h__

// this file is for gui element implementations, not for general outside usage


typedef struct GUIRenderParams {
	Vector2 offset; // parent-defined absolute from the top left of the screen
	Vector2 size; // of the parent's client area that the child lives in
	AABB2 clip; // absolute clipping region
	float baseZ; // accumulated absolute Z from the parent
} GUIRenderParams;







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


#define GUI_TEXT_ALIGN_LEFT 0x00
#define GUI_TEXT_ALIGN_RIGHT 0x01
#define GUI_TEXT_ALIGN_CENTER 0x02


void GUI_Box_(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	float width, 
	Color4* borderColor
);

void GUI_BoxFilled_(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	float width, 
	Color4* borderColor, 
	Color4* bgColor
);

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

#define GUI_TextLineCentered(a,b, c,d, e,f,g) GUI_TextLineCentered_(gm, a,b, c,d, e,f,g)



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


void gui_drawTriangle(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height, 
	float rotation,
	AABB2* clip, 
	float z, 
	Color4* bgColor
);

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
void GUI_SetActive_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*));
void* GUI_GetData_(GUIManager* gm, void* id);
void GUI_SetData_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*));


Vector2 GUI_MousePos_(GUIManager* gm);
#define GUI_MousePos() GUI_MousePos_(gm)

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

// returns true if clicked
int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char* text);

// returns true if toggled on
int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char* text, int* state);

// returns true if checked
int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, char* label, int* state);

// returns true if *this* radio button is active
int GUI_RadioButton_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault);

// returns 1 on change
int GUI_FloatSlider_(GUIManager* gm, void* id, Vector2 tl, float width, float min, float max, float incr, int prec, float* value);
#define GUI_FloatSlider(a,b,c,d,e,f,g,h) GUI_FloatSlider_(gm, a,b,c,d,e,f,g,h)

// returns 1 on change
int GUI_IntSlider_(GUIManager* gm, void* id, Vector2 tl, float width, long min, long max, long* value);
#define GUI_IntSlider(a,b,c,d,e,f) GUI_IntSlider_(gm, a,b,c,d,e,f)

// returns 1 when the value changes _due to this control_
int GUI_OptionSlider_(GUIManager* gm, void* id, Vector2 tl, float width, char** options, int* selectedOption);
#define GUI_OptionSlider(a,b,c,d,e) GUI_OptionSlider_(gm, a,b,c,d,e)

// filter all input before accepting it
void GUI_Edit_SetFilter_(GUIManager* gm, void* id, GUIEditFilterFn fn, void* data);
#define GUI_Edit_SetFilter(a, b, c) GUI_Edit_SetFilter_(gm, a, b, c)

// synthesizes an input event for the given edit control
int GUI_Edit_Trigger_(GUIManager* gm, void* id, GUIString* str, int c);
#define GUI_Edit_Trigger(a, b, c) GUI_Edit_Trigger_(gm, a, b, c)

// returns true on a change
int GUI_Edit_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, GUIString* s);
#define GUI_Edit(a, b, c, d) GUI_Edit_(gm, a, b, c, d)

// returns true on a change
int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, long* num);

// returns true on a change
int GUI_SelectBox_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char** options, int* selectedOption);


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
#define GUI_Rect(a, b, c) GUI_BoxFilled_(gm, a, b, 0, &(Color4){0,0,0,0}, c)

void GUI_CircleFilled_(
	GUIManager* gm, 
	Vector2 center, 
	float radius, 
	float borderWidth, 
	Color4* borderColor, 
	Color4* bgColor
);

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

int GUI_PointInBoxV_(GUIManager* gm, Vector2 tl, Vector2 size, Vector2 testPos);
int GUI_PointInBox_(GUIManager* gm, AABB2 box, Vector2 testPos);








#endif // __EACSMB_gui_internal_h__
