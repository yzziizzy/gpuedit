#ifndef __gpuedit_highlight_h__
#define __gpuedit_highlight_h__


#include <stddef.h>


#include "ds.h"
#include "common_math.h"



typedef struct TextStyleAtom {
	unsigned char length;
	union {
		unsigned char styleIndex;
	};
} TextStyleAtom;









struct Buffer;
typedef struct Buffer Buffer;
struct BufferLine;
typedef struct BufferLine BufferLine;

typedef struct hlinfo {
// 	void (*getPrevLine)(struct hlinfo*, char** /*txt*/, size_t* /*len*/);
	int  (*getNextLine)(struct hlinfo*, char** /*txt*/, size_t* /*len*/);
	void (*writeSection)(struct hlinfo*, unsigned char /*style*/, unsigned char/*length*/);
// 	void (*rewindStyle)(struct hlinfo*, int /*chars*/);
	
	// read-only
	ptrdiff_t dirtyLines;
	
	
	// private
	Buffer* b;
	BufferLine* readLine; 
	BufferLine* writeLine; 
	int writeCol;
	
	
} hlinfo;



typedef struct StyleInfo {
	int index;
	int category;
	Vector4 fgColorDefault; // temporary colors
	Vector4 bgColorDefault;
	Vector4 fgSelColorDefault; // temporary colors
	Vector4 bgSelColorDefault;
	char* name;
	
	int underline : 1;
	int bold : 1;
	int italic : 1;
	int useFGDefault : 1;
	int useBGDefault : 1;
	int useFGSelDefault : 1;
	int useBGSelDefault : 1;
} StyleInfo;

typedef struct Highlighter {
	
	void (*refreshStyle)(struct Highlighter*, hlinfo*);
	
	// for listing in the style editor
// 	void (*getStyleNames)(char** /*nameList*/, size_t* /*len*/);
// 	void (*getStyleDefaults)(char** /*nameList*/, size_t* /*len*/);
	
	StyleInfo* styles;
	int numStyles;
	
	
} Highlighter;


typedef struct Highlighter_C {
	Highlighter hl;
	
	
	
	
} Highlighter_C;






void initCStyles(Highlighter* hl);


#pragma magic [random] baz()
#pragma magic [random] foo()
#pragma magic [highlighter] bar()
#pragma magic [highlighter] zap() 

#endif // __gpuedit_highlight_h__
