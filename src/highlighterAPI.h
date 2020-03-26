#ifndef __gpuedit_highlighterAPI_h__
#define __gpuedit_highlighterAPI_h__


typedef struct Allocator {
	void* (*malloc)(struct Allocator*, size_t);
	void* (*calloc)(struct Allocator*, size_t);
	void* (*realloc)(struct Allocator*, void*, size_t);
	void  (*free)(struct Allocator*, void*);
	
	char* (*strdup)(struct Allocator*, char*);
	char* (*strndup)(struct Allocator*, char*, size_t);
} Allocator;


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


typedef struct HLContext {
	Allocator* alloc;
	
// 	void (*getPrevLine)(struct hlinfo*, char** /*txt*/, size_t* /*len*/);
	int  (*getNextLine)(struct HLContext*, char** /*txt*/, size_t* /*len*/);
	void (*writeSection)(struct HLContext*, unsigned char /*style*/, unsigned char/*length*/);
// 	void (*rewindStyle)(struct hlinfo*, int /*chars*/);
	
	// read-only
	ptrdiff_t dirtyLines;
	
	void* userData;
} HLContext;


typedef struct Color4f {
	float r, g, b, a;
} Color4f;



typedef struct StyleInfo {
	int index;
	int category;
	Color4f fgColor;
	Color4f bgColor;
	Color4f fgSelColor; 
	Color4f bgSelColor;
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
	uint32_t abiVersion;
	
	char* name;
	char* description;
	char* author;
	
	char* extensions; // semicolon separated
	
	
	uint64_t (*getStyleCount)();
	void (*getStyleNames)(char** /*nameList*/, uint64_t /*maxLen*/);
	void (*getStyleDefaults)(StyleInfo* /*styles*/, uint64_t /*maxLen*/);
	
	void (*refreshStyle)(HLContext*);
	
	void (*init)();
	void (*cleanup)();
	
} HighlighterPluginInfo;





#endif // __gpuedit_highlighterAPI_h__

