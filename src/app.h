#ifndef __gpuedit_app_h__
#define __gpuedit_app_h__

#include "common_math.h"
#include "common_gl.h"

#include "settings.h"
#include "window.h"
#include "commands.h"
#include "ui/gui.h"
#include "buffer.h"
#include "mainControl.h"



struct json_value_t;


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
	
	Settings* globalSettings;
	GeneralSettings* gs;
	
	GUIManager* gui;
	RenderPass* guiPass;

	MatrixStack view;
	MatrixStack proj;
	
	Matrix invView; // TODO: rename these
	Matrix invProj;
	Matrix mProjWorld;
	
	Vector2 cursorPos;
// 	Vector3 cursorPos;
	int cursorIndex;

	Vector2 mouseDownPos;
	
	int debugMode;
	
	float zoom;
	
	TextureAtlas* ta;
	
	InputFocusStack ifs;
	InputEventHandler* defaultInputHandlers;
	
	double lastFrameTime; // frameTime from the previous frame
	double lastFrameDrawTime; // the cost of rendering the previous frame, minus any sleeping
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

	
	MainControl* mc;
	BufferCache* bufferCache;
	
	GUI_Cmd* commands;
	
	
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


void AppState_ExecProcess(AppState* as, char* execPath, char* args[]);
struct child_process_info* AppState_ExecProcessPipe(char* execPath, char* args[]);
struct child_pty_info* AppState_ExecProcessPTY(AppState* as, char* execPath, char* args[]);


void execProcessPipe_buffer(char** args, char** buffer_out, size_t* size_out/*,int* code_out*/);
char* execProcessPipe_charpp(char** args, char*** charpp_out, size_t* n_out/*,int* code_out*/);
void execProcessPipe_bufferv(char*** args, char** buffer_out, size_t* size_out/*,int** code_out*/);
char* execProcessPipe_charppv(char*** args, char*** charpp_out, size_t* n_out/*,int** code_out*/);


void AppState_Init(AppState* as, int argc, char* argv[]);
void AppState_InitGL(XStuff* xs, AppState* as);


void appLoop(XStuff* xs, AppState* gs, InputState* is);
void SetUpPDP(AppState* as, PassDrawParams* pdp);
void initRenderLoop(AppState* gs);





#endif // __gpuedit_app_h__
