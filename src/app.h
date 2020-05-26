#ifndef __gpuedit_app_h__
#define __gpuedit_app_h__

#include "common_math.h"
#include "common_gl.h"

#include "settings.h"
#include "window.h"
#include "gui.h"
#include "settingsEditor.h"
#include "buffer.h"
#include "mainControl.h"


typedef struct AppScreen {
	
	float aspect;
	Vector2 wh;
	
	int resized;
	
} AppScreen;




typedef struct PerViewUniforms {
	Matrix view;
	Matrix proj;
} PerViewUniforms;


typedef struct PerFrameUniforms {
	float wholeSeconds;
	float fracSeconds;
} PerFrameUniforms;


typedef struct AppState {
	
	char* dataDir;
	char* worldDir;
	
	AppScreen screen;
	
	GlobalSettings globalSettings;
	
	GUIManager* gui;
	RenderPass* guiPass;

	MatrixStack view;
	MatrixStack proj;
	
	Matrix invView; // TODO: rename these
	Matrix invProj;
	Matrix mProjWorld;
	
	Vector2 cursorPos;
// 	Vector cursorPos;
	int cursorIndex;

	Vector2 mouseDownPos;
	
	int debugMode;
	
	float zoom;
	
	TextureAtlas* ta;

	InputFocusStack ifs;
	InputEventHandler* defaultInputHandlers;
	
	double frameTime; // ever incrementing time of the this frame
	double frameSpan; // the length of this frame, since last frame
	uint64_t frameCount; // ever incrementing count of the number of frames processed
	
	// performance counters
	struct {
		double preframe;
		double selection;
		double draw;
		double decal;
		double light;
		double shade;
		
	} perfTimes;
	
	struct {
		QueryQueue draw; 
		QueryQueue gui; 
		
	} queries;

	
	Buffer* currentBuffer;
	GUIMainControl* mc;
	
	Cmd* commands;
	
	
} AppState;

struct child_pty_info {
	int pid;
	int pty;
};

struct child_process_info {
	int pid;
	int child_stdin;
	int child_stdout;
	int child_stderr;
	FILE* f_stdin;
	FILE* f_stdout;
	FILE* f_stderr;
};


struct child_process_info* AppState_ExecProcessPipe(AppState* as, char* execPath, char* args[]);
struct child_pty_info* AppState_ExecProcessPTY(AppState* as, char* execPath, char* args[]);


void initApp(XStuff* xs, AppState* gs, int argc, char* argv[]);
void initAppGL(XStuff* xs, AppState* gs);

void AppState_UpdateSettings(AppState* as, GlobalSettings* gs);

void renderFrame(XStuff* xs, AppState* gs, InputState* is, PassFrameParams* pfp);
void appLoop(XStuff* xs, AppState* gs, InputState* is);


void initRenderLoop(AppState* gs);





#endif // __gpuedit_app_h__
