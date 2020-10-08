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


// different distances of syntax-aware jumping
#define FLAG_JUMP_1 0x01
#define FLAG_JUMP_2 0x02
#define FLAG_JUMP_3 0x04
#define FLAG_JUMP_4 0x08

// for paren/brace matching
#define FLAG_OPEN_GROUP  0x10
#define FLAG_CLOSE_GROUP 0x20

// for indentation
#define FLAG_OPEN_BLOCK  0x40
#define FLAG_CLOSE_BLOCK 0x80


typedef struct HLContext {
	Allocator* alloc;
	
	// these are callbacks provided by gpuedit to the module in order for
	//   the module to interact with the text
	int  (*getNextLine)(struct HLContext*, char** /*txt*/, size_t* /*len*/);
	void (*writeSection)(struct HLContext*, unsigned char /*style*/, unsigned char/*length*/);
	void (*writeFlags)(struct HLContext*, uint8_t* /*flag_buffer*/, size_t /*buffer_len*/);
	
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


// sent to gpuedit by the module to provide information about
//  a named set of highlighting features it has
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

