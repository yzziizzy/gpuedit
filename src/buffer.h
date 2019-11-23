#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__



#include "gui.h"
#include "font.h"

typedef struct BufferLine {
	
	int lineNum;
	
	size_t allocSz;
	size_t length; // of the text itself
	char* buf;
	
	struct BufferLine* prev, *next;
	
} BufferLine;


typedef struct Buffer {
	GUIHeader header;
	
	BufferLine* first, *last, *current; 
	
	int numLines;
	int curCol;
	int curLine;
	
	// TODO: move elsewhere
	GUIFont* font;
	
} Buffer;


struct GUIManager;
typedef struct GUIManager GUIManager;

BufferLine* Buffer_AddLineBelow(Buffer* b);
void Buffer_Draw(Buffer* b, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);


void Buffer_insertText(Buffer* b, char* text, size_t len);
Buffer* Buffer_New(GUIManager* gm);

#endif // __gpuedit_buffer_h__
