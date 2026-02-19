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
	RING(ClipboardClip*) stack[3];
	
	struct ClipBuffer os[3];
	
	char selfOwned[3]; // 1 if this app has ownership of the clipboard, 0 otherwise
	
	VEC(struct ClipCallback) onChange;
	
} Clipboard;


// the one and only
Clipboard* clipboard;



static void callOnChangeFns(int which) {
	VEC_EACH(&clipboard->onChange, i, cb) {
		if(cb.fn) cb.fn(which, cb.data);
	}
}


void Clipboard_PushBuffer(int which, Buffer* b) {
	
	ClipboardClip* cc = pcalloc(cc);
	
	cc->b = Buffer_Copy(b);
	Buffer_ToRawText(b, &cc->flatText, &cc->flatTextLen);
	cc->flatText[--cc->flatTextLen] = '\0';
	
	Clipboard_SendToOS(which, cc->flatText, cc->flatTextLen, 0);

	if(RING_LEN(&clipboard->stack[which]) == RING_ALLOC(&clipboard->stack[which])) {
		ClipboardClip* last = RING_TAIL(&clipboard->stack[which]);
		ClipboardClip_free(last);
	}
	
	RING_PUSH(&clipboard->stack[which], cc);
}


void Clipboard_PushRawText(int which, char* raw, size_t len) {
	if(!clipboard) return;
	
	ClipboardClip* cc = pcalloc(cc);
	
	cc->b = Buffer_New(NULL);
	cc->flatTextLen = len;
	cc->flatText = strndup(raw, len);
	Buffer_AppendRawText(cc->b, raw, len);

	Clipboard_SendToOS(which, cc->flatText, cc->flatTextLen, 0);

	if(RING_LEN(&clipboard->stack[which]) == RING_ALLOC(&clipboard->stack[which])) {
		ClipboardClip* last = RING_TAIL(&clipboard->stack[which]);
		ClipboardClip_free(last);
	}
	
	RING_PUSH(&clipboard->stack[which], cc);
}


void Clipboard_PeekRawText(int which, char** rawOut, size_t* lenOut) {
	if(!clipboard) return;
	
	if(clipboard->selfOwned[which]) {
		if(RING_LEN(&clipboard->stack[which]) == 0) goto NO_CLIPBOARD;
		
		ClipboardClip* cc = RING_TAIL(&clipboard->stack[which]);
		
		*lenOut = cc->flatTextLen;
		*rawOut = cc->flatText;
		return;
	}
	else {
		struct ClipBuffer cb = clipboard->os[which];
		if(cb.length == 0) goto NO_CLIPBOARD;
//		printf("[%d] %s (%ld)\n", which, cb.buf, cb.length);
		*lenOut = cb.length;
		*rawOut = cb.buf;
		return;
	}
	
NO_CLIPBOARD:
	*rawOut = NULL;
	*lenOut = 0;
}


Buffer* Clipboard_PeekBuffer(int which) {
	return RING_TAIL(&clipboard->stack[which])->b;
}



Buffer* Clipboard_PopBuffer(int which) {
	Buffer* b;
	
	if(clipboard->selfOwned[which]) {
//		printf(" paste: self owned\n");
		if(RING_LEN(&clipboard->stack[which]) == 0) return NULL;
		ClipboardClip* cc = RING_TAIL(&clipboard->stack[which]);
		b = cc->b;
	}
	else {
//		printf(" paste: os owned [%d]\n", which);
		if(clipboard->os[which].length == 0) return NULL;
//		printf("   no early return\n");
		b = Buffer_New(NULL);
		
//		printf("[%d] %s (%ld)\n", which, clipboard->os[which].buf, clipboard->os[which].length);
		Buffer_AppendRawText(b, clipboard->os[which].buf, clipboard->os[which].length);
	}
	
	
	
// 	VEC_pop1(&clipboard->stack);
	
// 	free(cc->flatText);
// 	free(cc);
	
	return b;
}


void Clipboard_SendToOS(unsigned int which, char* text, size_t len, int encoding) {
	Clipboard_SetFromOS(which, text, len, encoding);
// 	printf("'%.*s'",(int)len, text);
	clipboard->selfOwned[which] = 1;
	callOnChangeFns(which);
	
//	printf("self owned\n");
}


void Clipboard_SetFromOS(unsigned int which, char* text, size_t len, int encoding) {
	if(which > 2) return;
	struct ClipBuffer* b = clipboard->os + which;
	
	if(b->allocSize < len + 1) {
		b->allocSize = nextPOT(len + 1);
		b->buf = realloc(b->buf, b->allocSize);
//		printf("clip buffer sz: %ld\n", b->allocSize);
	}
//	printf("clip> %d '%.*s'\n", which, (int)len, text);
	memcpy(b->buf, text, len);
	b->buf[len] = 0;
	
	b->length = len;
	b->encoding = encoding;
	
	clipboard->selfOwned[which] = 0;
}

void Clipboard_GetFromOS(unsigned int which, char** text, size_t* len, int* encoding) {
	if(which > 2) return;
	struct ClipBuffer* b = clipboard->os + which;
	
	*text = b->buf;
	*len = b->length;
	if(encoding) *encoding = b->encoding;
}

void Clipboard_RegisterOnChange(void (*fn)(int,void*), void* data) {
	VEC_push(&clipboard->onChange, ((struct ClipCallback){(void*)fn, data}));
}


void Clipboard_Init() {
	pcalloc(clipboard);
	
	RING_INIT(&clipboard->stack[0], 1);
	RING_INIT(&clipboard->stack[1], 1);
	RING_INIT(&clipboard->stack[2], 1);
}

void Buffer_DecRef(Buffer* b);
void Buffer_Delete(Buffer* b);

void ClipboardClip_free(ClipboardClip* clip) {
	if(clip->b) {
		Buffer_DecRef(clip->b);
		Buffer_Delete(clip->b);
	}
	if(clip->flatText) free(clip->flatText);
	free(clip);
}



