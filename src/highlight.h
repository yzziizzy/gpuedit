#ifndef __gpuedit_highlight_h__
#define __gpuedit_highlight_h__


#include <stddef.h>




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


typedef struct Highlighter {
	
	void (*refreshStyle)(struct Highlighter*, hlinfo*);
	
	// for listing in the style editor
	void (*getStyleNames)(char** /*nameList*/, size_t* /*len*/);
// 	void (*getStyleDefaults)(char** /*nameList*/, size_t* /*len*/);
	
	
	
} Highlighter;



typedef struct Highlighter_C {
	Highlighter hl;
	
	
} Highlighter_C;








#endif // __gpuedit_highlight_h__
