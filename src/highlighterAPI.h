#ifndef __gpuedit_highlighterAPI_h__
#define __gpuedit_highlighterAPI_h__



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
	void* privatedata; // ?
	/*
	Buffer* b;
	BufferLine* readLine; 
	BufferLine* writeLine; 
	int writeCol;
	*/
	
} hlinfo;


typedef struct Color4 {
	float r, g, b, a;
} Color4;


typedef struct StyleInfo {
	int index;
	int category;
	Color4 fgColor;
	Color4 bgColor;
	Color4 fgSelColor; 
	Color4 bgSelColor;
	char* name;
	
	int underline : 1;
	int bold : 1;
	int italic : 1;
	int useFGDefault : 1;
	int useBGDefault : 1;
	int useFGSelDefault : 1;
	int useBGSelDefault : 1;
} StyleInfo;



typedef struct HighlighterPluginInfo {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint32_t reserved_1;
	
	char* name;
	
	void (*getStyleNames)(char** /*nameList*/, size_t* /*len*/);
	void (*getStyleDefaults)(char** /*nameList*/, size_t* /*len*/);
	
	void (*refreshStyle)(struct Highlighter*, hlinfo*);
	
	void (*init)();
	void (*cleanup)();
	
} HighlighterPluginInfo;





#endif // __gpuedit_highlighterAPI_h__
