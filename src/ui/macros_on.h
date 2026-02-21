



#define C(r,g,b)  (&((Color4){r,g,b,1.0}))
#define C4(r,g,b,a)  (&((Color4){r,g,b,a}))
#define V(_x,_y) ((Vector2){.x=(_x),.y=(_y)})
#define Z(a) gm->curZ += (a);

#define S(...) S_N(PP_NARG(__VA_ARGS__), __VA_ARGS__) 
#define S_N(n, a, ...) CAT(S_, n)(a __VA_OPT__(,) __VA_ARGS__) 
#define S_1(a) (a)
#define S_2(a, b) ((vec2){(a), (b)}) 
#define SV(a) ((vec2){(a).x, (a).y}) 


#define ID(a) ((void*)(a))

#define STATE_MAX_VALUE 4
#define STATE_DISABLED 3
#define STATE_ACTIVE 2
#define STATE_HOT    1
#define STATE_NORMAL 0


#define DEFAULTS(type, var) \
	type var[STATE_MAX_VALUE] = { \
		gm->defaults.opts.type[0], \
		gm->defaults.opts.type[1], \
		gm->defaults.opts.type[2], \
		gm->defaults.opts.type[3] \
	}


#define CUR_STATE(id) (gm->activeID == (id) ? STATE_ACTIVE : (gm->hotID == (id) ? STATE_HOT : STATE_NORMAL))

#define IS_HOT(id) (gm->hotID == (id))
#define IS_ACTIVE(id) (gm->activeID == (id))

#define HOT(id) GUI_SetHot_(gm, id, NULL, NULL)
#define ACTIVE(id) GUI_SetActive_(gm, id, NULL, NULL)

#define HOVER_HOT(...) HOVER_HOT_n(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define HOVER_HOT_n(n, ...) CAT(HOVER_HOT_, n)(__VA_ARGS__)
#define HOVER_HOT_1(id) \
	if(GUI_MouseInside(tl, sz)) { \
		HOT(id); \
	}

#define HOVER_HOT_3(id, tl, sz) \
	if(GUI_MouseInside(tl, sz)) { \
		HOT(id); \
	}
	

#define CLICK_HOT_TO_ACTIVE(...) CLICK_HOT_TO_ACTIVE_n(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define CLICK_HOT_TO_ACTIVE_n(n, ...) CAT(CLICK_HOT_TO_ACTIVE_, n)(gm, __VA_ARGS__)


#define MOUSE_DOWN_ACTIVE(id) \
	if(GUI_MouseWentDown(1)) { \
		ACTIVE(id); \
		GUI_CancelInput(); \
	}
	

#define CLAMP(min, val, max) val = MIN(MAX(min, val), max)


