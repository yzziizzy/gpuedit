#ifndef __EACSMB_ui_animations_pulse_h__
#define __EACSMB_ui_animations_pulse_h__


struct GUIAnimation;
typedef struct GUIAnimation GUIAnimation;

struct gui_anim_vtbl {
	void (*UpdatePos)(GUIAnimation* ga, GUIRenderParams* grp, PassFrameParams* pfp);
	void (*Start)(GUIAnimation* ga);
	void (*Stop)(GUIAnimation* ga);
	void (*Pause)(GUIAnimation* ga);
	void (*UnPause)(GUIAnimation* ga);
};



struct GUIAnimation {
	GUIHeader* target;
	
	char paused;
	
	struct gui_anim_vtbl* vt;
	
};


typedef struct GUIAnimPulse {
	GUIAnimation header;
	
	float speed;
	float magnitude;
	float t;
	
} GUIAnimPulse;


GUIAnimPulse* GUIAnimPulse_new(GUIHeader* target, float speed, float magnitude);



static inline void GUIAnimation_UpdatePos(GUIAnimation* ga, GUIRenderParams* grp, PassFrameParams* pfp) {
	if(ga->vt->UpdatePos)
		(*ga->vt->UpdatePos)(ga, grp, pfp);
	// animations have no default action
}



#endif // __EACSMB_ui_animations_pulse_h__
