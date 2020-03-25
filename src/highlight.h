#ifndef __gpuedit_highlight_h__
#define __gpuedit_highlight_h__


#include <stddef.h>


#include "sti/sti.h"
#include "common_math.h"


#include "highlighterAPI.h"



typedef struct HLContextInternal {
	HLContext ctx;
	
	Buffer* b;
	BufferLine* readLine; 
	BufferLine* writeLine; 
	int writeCol;
	
} HLContextInternal;


typedef struct Highlighter {
	HighlighterPluginInfo* plugin;
	
	StyleInfo* styles;
	int numStyles;
	
	
} Highlighter;


typedef struct HighlighterModule {
	char* path;
	void* libHandle;
	
	uint64_t numHighlighters;
	HighlighterPluginInfo** highlighters;
} HighligherModule;


typedef struct HighlighterManager {
	VEC(HighligherModule*) modules;
	VEC(HighlighterPluginInfo*) plugins;
	
	
	
} HighligherManager;





/*
typedef struct Highlighter_C {
	Highlighter hl;
	
	
	
	
} Highlighter_C;*/

// init
// getPlugins
// cleanup





void Highlighter_LoadStyles(Highlighter* h, char* path);
void Highlighter_PrintStyles(Highlighter* h);


#pragma magic [random] baz()
#pragma magic [random] foo()
#pragma magic [highlighter] bar()
#pragma magic [highlighter] zap() 

#endif // __gpuedit_highlight_h__
