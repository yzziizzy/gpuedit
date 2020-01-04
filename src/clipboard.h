#ifndef __gpuedit_clipboard_h__
#define __gpuedit_clipboard_h__


#include "sti/sti.h"



struct Buffer;
typedef struct Buffer Buffer;



enum {
	CLIP_PRIMARY = 0,
	CLIP_SECONDARY,
	CLIP_SELECTION,
};


typedef struct ClipboardClip {
	Buffer* b;
	char* flatText;
	size_t flatTextLen;
} ClipboardClip;







// extern Clipboard* clipboard;


void Clipboard_Init();


void Clipboard_PushBuffer(Buffer* b);
void Clipboard_PushRawText(char* raw, size_t len);
Buffer* Clipboard_PeekBuffer();
Buffer* Clipboard_PopBuffer();

void Clipboard_SendToOS(unsigned int which, char* text, size_t len, int encoding);

// called by functions dealing with X; not for normal usage
void Clipboard_SetFromOS(unsigned int which, char* text, size_t len, int encoding);
void Clipboard_GetFromOS(unsigned int which, char** text, size_t* len, int* encoding);
void Clipboard_RegisterOnChange(void (*fn)(int,void*), void* data);

#endif // __gpuedit_clipboard_h__
