#ifndef __gpuedit_fileBrowser_h__
#define __gpuedit_fileBrowser_h__

#include <sys/stat.h>

#include "gui.h"


typedef struct GUIFileBrowserEntry {
	char* name;
// 	int type;
	mode_t perms;
	
	char* owner;
	uid_t ownerID;
	char* group;
	gid_t groupID;
	
	uint64_t size;
	
	
	
	unsigned int type : 4; // unknown, reg, dir, symlink
	unsigned int hasStats: 1;
	unsigned int isAlreadyOpen : 1;
	unsigned int isSelected : 1;
	
} GUIFileBrowserEntry;


typedef struct GUIFileBrowser {
	GUIHeader header;
	
	GUIWindow* scrollbar;
	float sbMinHeight;
	intptr_t scrollOffset;
	
	intptr_t cursorIndex;
	intptr_t numSelected;
	
	char* curDir;
	
	VEC(GUIFileBrowserEntry) entries;
	
	void* onChooseData;
	void (*onChoose)(void*, char**, intptr_t);
	
} GUIFileBrowser;



GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path);
void GUIFileBrowser_Destroy(GUIFileBrowser* w);

void GUIFileBrowser_Refresh(GUIFileBrowser* w);



#endif // __gpuedit_fileBrowser_h__
