
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "app.h"
#include "gui.h"
#include "shader.h"

#include "c_json/json.h"
#include "json_gl.h"


extern RenderPipeline* rpipe;







void initRenderLoop(AppState* as) {
	
	// timer queries

	query_queue_init(&as->queries.gui);
	
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	glBindTexture(GL_TEXTURE_2D, 0);
}





void cleanUpView(XStuff* xs, AppState* as, InputState* is) {
	msPop(&as->view);
	msPop(&as->proj);
}



void SetUpPDP(AppState* as, PassDrawParams* pdp) {
	
	pdp->mWorldView = msGetTop(&as->view);
	pdp->mViewProj = msGetTop(&as->proj);
	
	pdp->mProjView = &as->invProj;
	pdp->mViewWorld = &as->invView;
	
	mInverse(&pdp->mViewProj, &as->invProj);
	mInverse(&pdp->mWorldView, &as->invView);
	
	pdp->eyeVec = as->eyeDir;
	pdp->eyePos = as->eyePos;
	pdp->targetSize = (Vector2i){as->screen.wh.x, as->screen.wh.y};
	pdp->timeSeconds = (float)(long)as->frameTime;
	pdp->timeFractional = as->frameTime - pdp->timeSeconds;
	
}





#define PF_START(x) as->perfTimes.x = getCurrentTime()
#define PF_STOP(x) as->perfTimes.x = timeSince(as->perfTimes.x)

void drawFrame(XStuff* xs, AppState* as, InputState* is) {
	

	PassDrawParams pdp;
	//pdp.mWorldView = msGetTop(&as->view);
	//pdp.mViewProj = msGetTop(&as->proj);
	
	SetUpPDP(as, &pdp);
	
	
	PassFrameParams pfp;
	pfp.dp = &pdp;
	pfp.timeElapsed = as->frameSpan;
	pfp.appTime = as->frameTime; // this will get regenerated from save files later
	pfp.wallTime = as->frameTime;
	
	
		
	
	glViewport(0, 0, as->screen.wh.x, as->screen.wh.y);
	
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW); // this is backwards, i think, because of the scaling inversion for z-up
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	query_queue_start(&as->queries.gui);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	
	
	glDisable(GL_DEPTH_TEST);
	RenderPass_preFrameAll(as->guiPass, &pfp);
	RenderPass_renderAll(as->guiPass, &pfp.dp);
	RenderPass_postFrameAll(as->guiPass);
	glEnable(GL_DEPTH_TEST);
	
	glDisable(GL_BLEND);
	
	query_queue_stop(&as->queries.gui);

	
	cleanUpView(xs, as, is);
	
	
	glXSwapBuffers(xs->display, xs->clientWin);
}



