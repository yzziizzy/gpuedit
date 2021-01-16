#ifndef __gputk_textf_h__
#define __gputk_textf_h__


typedef struct GUITextF {
	GUIHeader header;
	
	char* buffer;
	size_t bufferLen;
	char* fmt;
	void* args;
	
	char autoRefresh;
	// align, height, width wrapping
	
	
} GUITextF;



GUITextF* GUITextF_new(GUIManager* gm);

void GUITextF_setString(GUITextF* w, char* fmt, void* args);
void GUITextF_refresh(GUITextF* w);



#endif // __gputk_textf_h__
