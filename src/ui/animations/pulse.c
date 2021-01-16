#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#include "../gui.h"
#include "../gui_internal.h"





static void updatePos(GUIAnimPulse* ga, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	ga->t = fmod(ga->t + (pfp->timeElapsed * ga->speed), 2*3.141592653589793238462);
// 	ga->t = fmod(ga->t + (pfp->gameTimeElapsed * ga->speed), 2*3.141592653589793238462);
	
	ga->header.target->scale = (sin(ga->t) * .5 + .5) * ga->magnitude;
	
}





GUIAnimPulse* GUIAnimPulse_new(GUIHeader* target, float speed, float magnitude) {
	GUIAnimPulse* ga;
	
	static struct gui_anim_vtbl static_vt = {
		.UpdatePos = (void*)updatePos,
	};
	
	pcalloc(ga);
	ga->header.target = target;
	ga->header.vt = &static_vt;
	
	ga->speed = speed;
	ga->magnitude = magnitude;
	ga->t = 0;
	
	return ga;
}




