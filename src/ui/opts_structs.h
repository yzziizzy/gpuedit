#ifndef __gputk__opts_structs_h__
#define __gputk__opts_structs_h__

/*
#define STATE_ACTIVE 2
#define STATE_HOT    1
#define STATE_NORMAL 0
*/

typedef struct GUIButtonOpts {
	struct {
		Color4 bg, border, text;
	} colors[3]; // normal, hot, active
	
	Vector2 size;
	float borderWidth;
	float fontSize;
	char* fontName;
} GUIButtonOpts;

typedef GUIButtonOpts GUIToggleButtonOpts;


typedef struct GUICheckboxOpts {
	struct {
		Color4 bg, border, text;
	} colors[3]; // normal, hot, active
	
	Vector2 labelSize;
	float boxSize;
	float borderWidth;
	float fontSize;
	char* fontName;
} GUICheckboxOpts;


typedef struct GUIRadioBoxOpts {
	struct {
		Color4 bg, border, text;
	} colors[3]; // normal, hot, active
	
	Vector2 labelSize;
	float boxSize;
	float borderWidth;
	float fontSize;
	char* fontName;
} GUIRadioBoxOpts;


typedef struct GUIFloatSliderOpts {
	struct {
		Color4 bg, bar, text;
	} colors[3]; // normal, hot, active
	
	Vector2 size;
	int precision;
	float fontSize;
	char* fontName;
} GUIFloatSliderOpts;


typedef struct GUIIntSliderOpts {
	struct {
		Color4 bg, bar, text;
	} colors[3]; // normal, hot, active
	
	Vector2 size;
	float fontSize;
	char* fontName;
} GUIIntSliderOpts;


typedef struct GUIOptionSliderOpts {
	struct {
		Color4 bg, bar, text;
	} colors[3]; // normal, hot, active
	
	Vector2 size;
	float fontSize;
	char* fontName;
} GUIOptionSliderOpts;


typedef struct GUISelectBoxOpts {
	struct {
		Color4 bg, border, text;
	} colors[3]; // normal, hot, active
	
	Vector2 labelSize;
	Vector2 size;
	float borderWidth;
	float fontSize;
	char* fontName;
} GUISelectBoxOpts;


typedef struct GUIEditOpts {
	struct {
		Color4 bg, border, text;
	} colors[3]; // normal, hot, active
	
	Color4 cursorColor;
	Color4 selectionBgColor;
	Vector2 size;
	float borderWidth;
	float fontSize;
	char* fontName;
} GUIEditOpts;


typedef struct GUIEditOpts GUIIntEditOpts;


// only unique structs. typedef'd ones are not included.
#define GUI_OPTS_STRUCTS_LIST \
	X(GUIButtonOpts) \
	X(GUICheckboxOpts) \
	X(GUIRadioBoxOpts) \
	X(GUIFloatSliderOpts) \
	X(GUIIntSliderOpts) \
	X(GUIOptionSliderOpts) \
	X(GUISelectBoxOpts) \
	X(GUIEditOpts) \





#endif //__gputk__opts_structs_h__
