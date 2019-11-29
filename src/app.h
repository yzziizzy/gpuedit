
#ifndef __EACSMB_GAME_H__
#define __EACSMB_GAME_H__

#include "common_math.h"
#include "common_gl.h"

#include "settings.h"
#include "window.h"
#include "gui.h"
#include "buffer.h"


typedef struct AppScreen {
	
	float aspect;
	Vector2 wh;
	
	int resized;
	
} AppScreen;


typedef struct AppSettings {
	
	float keyRotate;
	float keyScroll;
	float keyZoom;
	
	float mouseRotate;
	float mouseScroll;
	float mouseZoom;
	
} AppSettings;




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
	
	AppSettings settings;
	GlobalSettings globalSettings;
	
	GUIManager* gui;
	RenderPass* guiPass;

	MatrixStack view;
	MatrixStack proj;
	
	Matrix invView; // TODO: rename these
	Matrix invProj;
	Matrix mProjWorld;
	
	double nearClipPlane;
	double farClipPlane;
	
	Vector eyePos;
	Vector eyeDir;
	Vector eyeUp;
	Vector eyeRight;
	
	Vector2 cursorPos;
// 	Vector cursorPos;
	int cursorIndex;

	Vector2 mouseDownPos;
	
	int debugMode;
	
	float zoom;
	

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
	GUITabControl* tc;
	
} AppState;




void initApp(XStuff* xs, AppState* gs);
void initAppGL(XStuff* xs, AppState* gs);


void renderFrame(XStuff* xs, AppState* gs, InputState* is, PassFrameParams* pfp);
void appLoop(XStuff* xs, AppState* gs, InputState* is);


void initRenderLoop(AppState* gs);





#endif // __EACSMB_GAME_H__
