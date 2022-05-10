



#define C(r,g,b)  (&((Color4){r,g,b,1.0}))
#define C4(r,g,b,a)  (&((Color4){r,g,b,a}))
#define V(_x,_y) ((Vector2){(_x),(_y)})

#define HOT(id) GUI_SetHot_(gm, id, NULL, NULL)
#define ACTIVE(id) GUI_SetActive_(gm, id, NULL, NULL)

#define HOVER_HOT(id) \
	if(GUI_MouseInside(tl, sz)) { \
		HOT(id); \
	}
	
#define CLICK_HOT_TO_ACTIVE(id) \
	if(gm->hotID == id) { \
		if(GUI_MouseWentDown(1)) { \
			ACTIVE(id); \
			GUI_CancelInput(); \
		} \
	}

#define MOUSE_DOWN_ACTIVE(id) \
	if(GUI_MouseWentDown(1)) { \
		ACTIVE(id); \
		GUI_CancelInput(); \
	}
	

#define CLAMP(min, val, max) val = MIN(MAX(min, val), max)


