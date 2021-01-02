#ifndef __EACSMB_pass_h__
#define __EACSMB_pass_h__


#include "common_gl.h"
#include "common_math.h"

#include "sti/sti.h"

#include "fbo.h"
#include "shader.h"


// TODO: check alignment for ubo
typedef struct PassDrawParams {
	Matrix* mWorldView;
	Matrix* mViewProj;
	Matrix* mWorldProj;
	
	// inverse
	Matrix* mViewWorld;
	Matrix* mProjView;
	Matrix* mProjWorld;
	
	Vector3 eyeVec;
	Vector3 eyePos;
	Vector3 sunVec;
	
	Vector3 vEyePos;
	Vector3 vLookDir;
	Vector3 vEyeSun;
	Vector3 vSunPos;
	
	float timeSeconds;
	float timeFractional;
	float spinner;
	
	Vector2i targetSize;
} PassDrawParams;

typedef struct PassFrameParams {
	PassDrawParams* dp;
	
	double timeElapsed; // time since last frame
	double appTimeElapsed; // like above but gets paused, persisted on save, etc
	double appTime; // gets paused, persisted on save, etc
	double wallTime; // from the first frame rendered this session
} PassFrameParams;



typedef struct DrawTimer {
	char timerLen; // size of history buffer, max 16
	char timerNext;
	char timerUsed;
	char timerHistIndex;
	GLuint timerStartIDs[5];
	GLuint timerEndIDs[5];
	float timerHistory[16];
	
	// results values 
	float timerAvg;
	float timerMin;
	float timerMax;
} DrawTimer;




struct PassDrawable;

typedef void (*PassDrawFn)(void* data, GLuint progID, PassDrawParams* dp);


typedef struct PassDrawable {
	
	char* name;
	
	GLuint diffuseUL;
	GLuint normalsUL;
	GLuint lightingUL;
	GLuint depthUL;
	
	// forward matrices
	GLint ul_mWorldView;
	GLint ul_mViewProj;
	GLint ul_mWorldProj;
	
	// inverse matrices
	GLint ul_mViewWorld;
	GLint ul_mProjView;
	GLint ul_mProjWorld;
	
	GLint ul_timeSeconds;
	GLint ul_timeFractional;
	
	GLint ul_targetSize;
	
	ShaderProgram* prog; 
	void* data;
	
	// where uniform buffers would be set up
	void (*preFrame)(PassFrameParams*, void*);
	
	// might be called many times
	PassDrawFn draw;
	
	// where circular buffers are rotated
	void (*postFrame)(void*);
	
	DrawTimer timer;
} PassDrawable;




typedef struct RenderPass {
	
	char clearColor;
	char clearDepth;
	
	char fboIndex;
	GLuint readBuffer;
	GLuint drawBuffer;
	// fbo config, texture bindings, etc
	
	VEC(PassDrawable*) drawables;
	
} RenderPass;


typedef struct RenderPipelineFBOConfig {
	GLenum attachment;
	int texIndex;
} RenderPipelineFBOConfig;


typedef struct RenderPipeline {
	
	Vector2i viewSz;
	
	Vector4 clearColor;
	
	// fbo's
	FBOTexConfig* fboTexConfig;
	GLuint* backingTextures;
	
	VEC(RenderPipelineFBOConfig*) fboConfig;
	Framebuffer** fbos;
	
	VEC(RenderPass*) passes;
	
} RenderPipeline;


void initRenderPipeline();


int RenderPass_addDrawable(RenderPass* rp, PassDrawable* d);
PassDrawable* Pass_allocDrawable(char* name);

void RenderPass_init(RenderPass* pass); 
void RenderPipeline_addShadingPass(RenderPipeline* rpipe, char* shaderName); 

void RenderPipeline_renderAll(RenderPipeline* rp, PassFrameParams* pfp);
void RenderPass_renderAll(RenderPass* pass, PassDrawParams* pdp);
void RenderPass_preFrameAll(RenderPass* pass, PassFrameParams* pfp);
void RenderPass_postFrameAll(RenderPass* pass);


void RenderPipeline_init(RenderPipeline* rp);
void RenderPipeline_setFBOTexConfig(RenderPipeline* rp, FBOTexConfig* texcfg);
void RenderPipeline_setFBOConfig(RenderPipeline* rp, RenderPipelineFBOConfig* cfg, char* name);
void RenderPipeline_rebuildFBOs(RenderPipeline* rp, Vector2i sz);
void RenderPipeline_destroy(RenderPipeline* rp);

GLuint RenderPipeline_getOutputTexture(RenderPipeline* rp);


// temporary hacky code
typedef void (*pass_callback)(void* data, PassFrameParams* pfp);
void RegisterPrePass(pass_callback cb, void* data, char* name);
void RemovePrePass(char* name);
void RenderAllPrePasses(PassFrameParams* pfp);





void DrawTimer_Init(DrawTimer* pd);
void DrawTimer_Start(DrawTimer* pd);
void DrawTimer_End(DrawTimer* pd);






#endif // __EACSMB_pass_h__
