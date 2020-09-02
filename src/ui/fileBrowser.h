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
	unsigned int isRoot: 1; // no parent folder
	
} GUIFileBrowserEntry;


// just the bare file/folder tree.
typedef struct GUIFileBrowserControl {
	GUIHeader header;
	
	float lineHeight;
	float leftMargin;
	// iconsize, font params, etc
	
	GUIWindow* scrollbar;
	float sbMinHeight;
	intptr_t scrollOffset;
	
	intptr_t cursorIndex;
	intptr_t numSelected;
	
	char* curDir;
	
	VEC(GUIFileBrowserEntry) entries;
	
	unsigned int treeView       : 1;
	unsigned int onlyDirs       : 1;
	unsigned int multipleSelect : 1;
	// click/dbl/right actions
	
	void* onChooseData;
	void (*onChoose)(void*, char**, intptr_t);
	
} GUIFileBrowserControl;




// the full-functioning dialog contents
typedef struct GUIFileBrowser {
	GUIHeader header;
	
	unsigned int showPathBar     : 1;
	unsigned int editablePathBar : 1;
	unsigned int multipleSelect  : 1;
	unsigned int onlyExisting    : 1;
	unsigned int allowDirs       : 1;
	unsigned int onlyDirs        : 1;
	unsigned int showNewFolder   : 1;
	unsigned int showNewFile     : 1;
	
	GUIFileBrowserControl* fbc;
	GUIEdit* filenameBar;
	GUIButton* newDirBtn;
	GUIButton* newFileBtn;
	GUIButton* acceptBtn; // Save/Open/etc
	GUIButton* cancelBtn;
	
	char* curDir;
	
	char* acceptButtonLabel;
	
} GUIFileBrowser;

GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path);
void GUIFileBrowser_Destroy(GUIFileBrowser* w);


GUIFileBrowserControl* GUIFileBrowserControl_New(GUIManager* gm, char* path);

void GUIFileBrowser_Refresh(GUIFileBrowser* w);
void GUIFileBrowserControl_Refresh(GUIFileBrowserControl* w);



#endif // __gpuedit_fileBrowser_h__
