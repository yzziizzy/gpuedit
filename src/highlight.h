#ifndef __gpuedit_highlight_h__
#define __gpuedit_highlight_h__


#include <stddef.h>


enum HighlightMode {
	HLMODE_PER_WORD = 0,
	HLMODE_PER_LINE,
	HLMODE_PER_CHUNK,
};



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
	enum HighlightMode mode;
	
// 	union {
// 		void (*AcceptWord)(struct Highlighter*, char*, size_t, TextStyleMeta*);
	
		// returns 0 when no more chunks are needed
// 		int (*AcceptLine)(struct Highlighter*, char*, size_t, TextStyleMeta*);
// 	};
	
} Highlighter;



typedef struct Highlighter_C {
	Highlighter hl;
	
	
} Highlighter_C;








#endif // __gpuedit_highlight_h__
