#ifndef __gpuedit_fileBrowser_h__
#define __gpuedit_fileBrowser_h__



#include "gui.h"


typedef struct GUIFileBrowserEntry {
	char* name;
	int type;
	int perms;
	char* owner;
	char* group;
	long size;
	
} GUIFileBrowserEntry;


typedef struct GUIFileBrowser {
	GUIHeader header;
	
	char* curDir;
	
	VEC(GUIFileBrowserEntry) entries;
	
} GUIFileBrowser;



GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* format, void* target, char type);




#endif // __gpuedit_fileBrowser_h__
