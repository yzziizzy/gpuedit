#ifndef __gputk_controls_h__
#define __gputk_controls_h__


/*  __________________
  //                 //
 //   Static Text   //
//_________________*/


// draws a single character from its font origin point
//void GUI_Char_(GUIManager* gm, int c, Vector2 origin, char* fontName, float size, Color4* color);
//#define GUI_Char(a,b,c,d,e) GUI_Char_(gm, a,b,c,d,e) 
//void GUI_CharFont_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color);
//#define GUI_CharFont(a,b,c,d,e) GUI_CharFont_(gm, a,b,c,d,e) 


void GUI_Double_(GUIManager* gm, vec2 tl, double d, int precision);
#define GUI_Double(...) GUI_Double_(gm, __VA_ARGS__)
void GUI_Integer_(GUIManager* gm, vec2 tl, int64_t i);
#define GUI_Integer(...) GUI_Integer_(gm, __VA_ARGS__)







/*  _____________
  //            //
 //   Shapes   //
//____________*/



// solid rectangle with no border
#define GUI_Rect(tl, sz, color) GUI_BoxFilled_(gm, tl, sz, 0, &(Color4){0,0,0,0}, color)


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


void GUI_BoxRounded_(GUIManager* gm, Vector2 tl, Vector2 sz, float radius, float width, Color4* borderColor, Color4* bgColor);
#define GUI_BoxRounded(...) GUI_BoxRounded_(gm, __VA_ARGS__)

void GUI_BoxInset_(GUIManager* gm, 
	Vector2 tl, Vector2 sz, 
	float cornerRadius, 
	float borderWidth, 
	Color4* borderColor_t, 
	Color4* borderColor_b, 
	Color4* borderColor_l, 
	Color4* borderColor_r, 
	Color4* bgTopColor,
	Color4* bgBottomColor
);
#define GUI_BoxInset(...) GUI_BoxInset_(gm, __VA_ARGS__)

// gradient background, for controls
// properly styled, pop-UP box, like a button
void GUI_StandardOutsetBox_(GUIManager* gm, Vector2 tl, Vector2 sz);
#define GUI_StandardOutsetBox(a,b) GUI_StandardOutsetBox_(gm, a,b)

// properly styled, raised-UP area, with window background
void GUI_StandardRaisedBox_(GUIManager* gm, Vector2 tl, Vector2 sz);
#define GUI_StandardRaisedBox(a,b) GUI_StandardRaisedBox_(gm, a,b)

// properly styled, sunk-IN area, with groove background
void GUI_StandardSunkenBox_(GUIManager* gm, Vector2 tl, Vector2 sz);
#define GUI_StandardSunkenBox(a,b) GUI_StandardSunkenBox_(gm, a,b)


// Linear gradients
void GUI_HGradient_(GUIManager* gm, Vector2 tl, Vector2 sz, Color4* color1, Color4* color2);
#define GUI_HGradient(a, b, c, d) GUI_HGradient_(gm, a, b, c, d)
void GUI_VGradient_(GUIManager* gm, Vector2 tl, Vector2 sz, Color4* color1, Color4* color2);
#define GUI_VGradient(a, b, c, d) GUI_VGradient_(gm, a, b, c, d)

// Radial gradient
void GUI_RGradient_(GUIManager* gm, Vector2 tl, Vector2 sz, Color4* color1, Color4* color2);
#define GUI_RGradient(a, b, c, d) GUI_RGradient_(gm, a, b, c, d)



void GUI_CircleFilled_(
	GUIManager* gm, 
	Vector2 center, 
	float radius, 
	float borderWidth, 
	Color4* borderColor, 
	Color4* bgColor
);

#define GUI_CircleFilled(a,b,c,d,e) GUI_CircleFilled_(gm, a,b,c,d,e);

void GUI_Line_(
	GUIManager* gm, 
	Vector2 p1, 
	Vector2 p2, 
	float width, 
	Color4* color
);
#define GUI_Line(a,b,c,d) GUI_Line_(gm, a,b,c,d);


void GUI_Triangle_(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height, 
	float rotation,
	Color4* bgColor
);
#define GUI_Triangle(...) GUI_Triangle_(gm, __VA_ARGS__)


/*  _______________
  //              //
 //   Controls   //
//______________*/

void GUI_Image_(GUIManager* gm, Vector2 tl, Vector2 sz, char* name);
#define GUI_Image(a,b,c) GUI_Image_(gm, a,b,c)

// NOT WHAT YOU WANT: only internal GUIManager textures
void GUI_GUITexture_(GUIManager* gm, Vector2 tl, Vector2 sz, int type, int index, GLuint id);
#define GUI_GUITexture(...) GUI_GUITexture_(gm, __VA_ARGS__)

// YES; WHAT YOU WANT: id is an index in the global AssetManager image array
void GUI_Texture_(GUIManager* gm, Vector2 tl, Vector2 sz, u32 id);
#define GUI_Texture(...) GUI_Texture_(gm, __VA_ARGS__)

// bare mechanics for self-rendered clickable areas
int GUI_Clickable_(GUIManager* gm, void* id, vec2 tl, vec2 sz, int button);
#define GUI_Clickable(...) GUI_Clickable_(gm, __VA_ARGS__)

// returns true if clicked
int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, char* text, GUIButtonOpts* o);
#define GUI_Button(a,b,c) GUI_Button_(gm, a,b,c, gm->defaults.opts.GUIButtonOpts)

int GUI_MoatButton_(GUIManager* gm, void* id, Vector2 tl, char* text, GUIButtonOpts* o);
#define GUI_MoatButton(a,b,c) GUI_MoatButton_(gm, a,b,c, gm->defaults.opts.GUIButtonOpts)



// NOTE: Only updates the text on the first frame
// returns 1 if clicked
int GUI_PrintfButton_(GUIManager* gm, void* id, Vector2 tl, GUIButtonOpts* o, char* fmt, ...);
#define GUI_PrintfButton(a,b,c, ...) GUI_PrintfButton_(gm, a,b, gm->defaults.opts.GUIButtonOpts, c __VA_OPT(,) __VA_ARGS__)

// returns true if toggled on
int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, char* text, int* state, GUIButtonOpts* o);
#define GUI_ToggleButton(a,b,c,d) GUI_ToggleButton_(gm, a,b,c,d, gm->defaults.opts.GUIButtonOpts)

// returns true if checked
int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, int* state, GUICheckboxOpts* o);
#define GUI_Checkbox(a,b,c,d) GUI_Checkbox_(gm, a,b,c, gm->defaults.opts.GUICheckboxOpts)
int GUI_CheckboxVCentered_(GUIManager* gm, void* id, vec2 tl, vec2 sz, int* state, GUICheckboxOpts* o);
#define GUI_CheckboxVCentered(a,b,c,d) GUI_CheckboxVCentered_(gm, a,b,c,d, gm->defaults.opts.GUICheckboxOpts)

// returns true if *this* radio button is active
int GUI_RadioBox_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault, GUIRadioBoxOpts* o);
#define GUI_RadioBox(a, b, c, d, e) GUI_RadioBox_(gm, a,b,c,d,e, gm->defaults.opts.GUIRadioBoxOpts)

// returns 1 on change
int GUI_FloatSlider_(GUIManager* gm, void* id, vec2 tl, vec2 sz, float min, float max, float incr, float* value, GUIFloatSliderOpts* o);
#define GUI_FloatSlider(a,b,c,d,e,f,g) GUI_FloatSlider_(gm, a,b,c,d,e,f,g, gm->defaults.opts.GUIFloatSliderOpts)

// returns 1 on change
int GUI_IntSlider_(GUIManager* gm, void* id, Vector2 tl, long min, long max, long* value, GUIIntSliderOpts* o);
#define GUI_IntSlider(a,b,c,d,e) GUI_IntSlider_(gm, a,b,c,d,e, gm->defaults.opts.GUIIntSliderOpts)

// returns 1 when the value changes _due to this control_
int GUI_OptionSlider_(GUIManager* gm, void* id, Vector2 tl, char** options, int* selectedOption, GUIOptionSliderOpts* o);
#define GUI_OptionSlider(a,b,c,d) GUI_OptionSlider_(gm, a,b,c,d, gm->defaults.opts.GUIOptionSliderOpts)

// filter all input before accepting it
void GUI_Edit_SetFilter_(GUIManager* gm, void* id, GUIEditFilterFn fn, void* data);
#define GUI_Edit_SetFilter(a, b, c) GUI_Edit_SetFilter_(gm, a, b, c)

// synthesizes an input event for the given edit control
int GUI_Edit_Trigger_(GUIManager* gm, void* id, GUIString* str, int c);
#define GUI_Edit_Trigger(a, b, c) GUI_Edit_Trigger_(gm, a, b, c)

// returns true on a change
int GUI_Edit_(GUIManager* gm, void* id, Vector2 tl, float width, GUIString* s, GUIEditOpts* o);
#define GUI_Edit(a,b,c,d) GUI_Edit_(gm, a,b,c,d, gm->defaults.opts.GUIEditOpts)

// returns true on a change
int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, float width, long* num, GUIIntEditOpts* o);
#define GUI_IntEdit(a,b,c,d) GUI_IntEdit_(gm, a,b,c,d, gm->defaults.opts.GUIIntEditOpts)

int GUI_FloatEdit_(GUIManager* gm, void* id, Vector2 tl, float width, float* num, GUIFloatEditOpts* o);
#define GUI_FloatEdit(a,b,c,d) GUI_FloatEdit_(gm, a,b,c,d, gm->defaults.opts.GUIFloatEditOpts)


// returns true on a change
int GUI_DropdownBox_(GUIManager* gm, void* id, Vector2 tl, float width, char** options, int* selectedOption, GUIDropdownBoxOpts* o);
#define GUI_DropdownBox(a,b,c,d,e) GUI_DropdownBox_(gm, a,b,c,d,e, gm->defaults.opts.GUIDropdownBoxOpts)


// returns true on a change
int GUI_ScrollwheelRegion_(GUIManager* gm, vec2 tl, vec2 sz, f32 range, f32 increment, f32* pos);
#define GUI_ScrollwheelRegion(a, b, c, d, e) GUI_ScrollwheelRegion_(gm, a, b, c, d, e)

// returns true on a change
int GUI_HScrollbar_(GUIManager* gm, void* id, vec2 tl, vec2 sz, f32 range, f32* pos, GUIScrollbarOpts* o);
#define GUI_HScrollbar(a, b, c, d, e) GUI_HScrollbar_(gm, a, b, c, d, e, gm->defaults.opts.GUIScrollbarOpts)

// returns true on a change
int GUI_VScrollbar_(GUIManager* gm, void* id, vec2 tl, vec2 sz, f32 range, f32* pos, GUIScrollbarOpts* o);
#define GUI_VScrollbar(a, b, c, d, e) GUI_VScrollbar_(gm, a, b, c, d, e, gm->defaults.opts.GUIScrollbarOpts)



#endif //  __gputk_controls_h__
