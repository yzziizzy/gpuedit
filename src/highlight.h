#ifndef __gpuedit_highlight_h__
#define __gpuedit_highlight_h__


#include <stddef.h>


#include "sti/sti.h"
#include "common_math.h"


#include "highlighterAPI.h"


// A nested structure passed to the module during text operations
// The module only knows about the first member, HLContext
// MODULES SHOULD NOT POKE AROUND IN INTERNAL MEMBERS.
typedef struct HLContextInternal {
	HLContext ctx;
	
	Buffer* b;
	
	struct {
		BufferLine* readLine; 
		BufferLine* writeLine; 
		intptr_t writeCol;
	} color, flags;
	
} HLContextInternal;


// used by gpuedit to hold a specific highlighter's data
typedef struct Highlighter {
	HighlighterPluginInfo* plugin; // fetched from the module
	
	StyleInfo* styles; // fetched from the module
	int numStyles;
	
	
} Highlighter;


// used by gpuedit to manage an .so with potentially multiple highlighters in it
typedef struct HighlighterModule {
	char* path;
	void* libHandle;
	
	uint64_t numHighlighters;
	HighlighterPluginInfo** highlighters;
} HighlighterModule;


// used by gpuedit to hold all the modules, highlighters, and plugins
typedef struct HighlighterManager {
	VEC(HighlighterModule*) modules;
	VEC(Highlighter*) plugins;
	
	HT(Highlighter*) extLookup;
} HighlighterManager;


void HighlighterManager_Init(HighlighterManager* hm);
void HighlighterManager_Destroy(HighlighterManager* hm);

HighlighterModule* Highlighter_LoadModule(HighlighterManager* hm, char* path);
void Highlighter_LoadStyles(Highlighter* h, char* path);
void Highlighter_PrintStyles(Highlighter* h);

Highlighter* HighlighterManager_ProbeExt(HighlighterManager* hm, char* filename);



#pragma magic [random] baz()
#pragma magic [random] foo()
#pragma magic [highlighter] bar()
#pragma magic [highlighter] zap() 

#endif // __gpuedit_highlight_h__
