#ifndef __gpuedit_clipboard_h__
#define __gpuedit_clipboard_h__


#include "sti/sti.h"



struct Buffer;
typedef struct Buffer Buffer;



enum {
	CLIP_PRIMARY = 0, // "regular" clipboard
	CLIP_SECONDARY, // secondary "regular" clipboard
	CLIP_SELECTION, // select/middle-click buffer
};


typedef struct ClipboardClip {
	Buffer* b;
	char* flatText;
	size_t flatTextLen;
} ClipboardClip;







// extern Clipboard* clipboard;


void Clipboard_Init();


void Clipboard_PushBuffer(int which, Buffer* b);
void Clipboard_PushRawText(int which, char* raw, size_t len);
void Clipboard_PeekRawText(int which, char** rawOut, size_t* lenOut);
Buffer* Clipboard_PeekBuffer(int which);
Buffer* Clipboard_PopBuffer(int which);

void Clipboard_SendToOS(unsigned int which, char* text, size_t len, int encoding);

// called by functions dealing with X; not for normal usage
void Clipboard_SetFromOS(unsigned int which, char* text, size_t len, int encoding);
void Clipboard_GetFromOS(unsigned int which, char** text, size_t* len, int* encoding);
void Clipboard_RegisterOnChange(void (*fn)(int,void*), void* data);

#endif // __gpuedit_clipboard_h__
