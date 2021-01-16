#ifndef __gputk_monitors_h__
#define __gputk_monitors_h__


typedef struct GUIValueMonitor {
	GUIHeader header;
	
	GUIText* text;
	
	char* format;
	void* target;
	char type;
	
	char* buffer;
	size_t bufferLen;
	
} GUIValueMonitor;



GUIValueMonitor* GUIValueMonitor_new(GUIManager* gm, char* format, void* target, char type);



#endif // __gputk_monitors_h__
