#ifndef __gputk_slider_h__
#define __gputk_slider_h__



// onchange
struct GUISlider;
typedef void (*GUISliderOnChangeFn)(struct GUISlider*, void*);


typedef struct GUISlider {
	GUIHeader header;
	InputEventHandler* inputHandlers;
	
	double min, max, step, value;
	
	float handleOffset; // in pixels
	char intOnly;
	char hasFocus;
	
	GUIText* textControl;
	
	GUISliderOnChangeFn onChange;
	void* onChangeData;
	
} GUISlider;


GUISlider* GUISlider_New(GUIManager* gm, double min, double max, double initialValue);

void GUISlider_SetDouble(GUISlider* ed, double dval); 
double GUISlider_GetDouble(GUISlider* ed);


#endif // __gputk_slider_h__
