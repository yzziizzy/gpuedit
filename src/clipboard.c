#include "stdlib.h"
#include "string.h"


#include <X11/X.h>
#include <X11/Xlib.h>


#include "clipboard.h"
#include "buffer.h"


struct ClipBuffer {
	char* buf;
	size_t length;
	size_t allocSize;
	int encoding;
};


struct ClipCallback {
	void (*fn)(unsigned int, void*);
	void* data;
};


typedef struct Clipboard {
	VEC(ClipboardClip*) stack;
	
	
	struct ClipBuffer os[3];
	
	char selfOwned; // 1 if this app has ownership of the clipboard, 0 otherwise
	
	VEC(struct ClipCallback) onChange;
	
} Clipboard;


// the one and only
Clipboard* clipboard;



static void callOnChangeFns(int which) {
	VEC_EACH(&clipboard->onChange, i, cb) {
		if(cb.fn) cb.fn(which, cb.data);
	}
}


void Clipboard_PushBuffer(Buffer* b) {
	
	ClipboardClip* cc = pcalloc(cc);
	
	cc->b = Buffer_Copy(b);
	Buffer_ToRawText(b, &cc->flatText, &cc->flatTextLen);
	
	VEC_PUSH(&clipboard->stack, cc);
}


void Clipboard_PushRawText(char* raw, size_t len) {
	if(!clipboard) return;
	
	ClipboardClip* cc = pcalloc(cc);
	
	cc->b = Buffer_New();
	cc->flatTextLen = len;
	cc->flatText = strndup(raw, len);
	Buffer_AppendRawText(cc->b, raw, len);
	
	VEC_PUSH(&clipboard->stack, cc);
	

}


Buffer* Clipboard_PeekBuffer() {
	return VEC_TAIL(&clipboard->stack)->b;
}


Buffer* Clipboard_PopBuffer() {
	
	ClipboardClip* cc = VEC_TAIL(&clipboard->stack);
	Buffer* b = cc->b;
	
	VEC_POP1(&clipboard->stack);
	
	free(cc->flatText);
	free(cc);
	
	return b;
}


void Clipboard_SendToOS(unsigned int which, char* text, size_t len, int encoding) {
	Clipboard_SetFromOS(which, text, len, encoding);
	printf("'%.*s'",len, text);
	clipboard->selfOwned = 1;
	callOnChangeFns(which);
}


void Clipboard_SetFromOS(unsigned int which, char* text, size_t len, int encoding) {
	if(which > 2) return;
	struct ClipBuffer* b = clipboard->os + which;
	
	if(b->allocSize < len + 1) {
		b->allocSize = nextPOT(len + 1);
		b->buf = realloc(b->buf, b->allocSize);
	}
	
	memcpy(b->buf, text, len);
	b->buf[len] = 0;
	
	b->length = len;
	b->encoding = encoding;
	
	clipboard->selfOwned = 0;
}

void Clipboard_GetFromOS(unsigned int which, char** text, size_t* len, int* encoding) {
	if(which > 2) return;
	struct ClipBuffer* b = clipboard->os + which;
	
	*text = b->buf;
	*len = b->length;
	if(encoding) *encoding = b->encoding;
}

void Clipboard_RegisterOnChange(void (*fn)(int,void*), void* data) {
	VEC_PUSH(&clipboard->onChange, ((struct ClipCallback){fn, data}));
}


void Clipboard_Init() {
	pcalloc(clipboard);
}




