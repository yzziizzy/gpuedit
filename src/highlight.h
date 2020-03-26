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
} HighlighterModule;


typedef struct HighlighterManager {
	VEC(HighlighterModule*) modules;
	VEC(Highlighter*) plugins;
	
	
	
} HighlighterManager;


HighlighterModule* Highlighter_LoadModule(HighlighterManager* hm, char* path);





void Highlighter_LoadStyles(Highlighter* h, char* path);
void Highlighter_PrintStyles(Highlighter* h);


#pragma magic [random] baz()
#pragma magic [random] foo()
#pragma magic [highlighter] bar()
#pragma magic [highlighter] zap() 

#endif // __gpuedit_highlight_h__
